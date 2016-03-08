/* A database containing ecurves
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak, Manuel Landesfeind
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "uproc/error.h"
#include "uproc/database.h"

#include "uproc/ecurve.h"
#include "uproc/dict.h"
#include "uproc/matrix.h"
#include "uproc/idmap.h"
#include "uproc/substmat.h"

/** Struct defining a database **/
struct uproc_database_s
{
    /** Data describing the database.
     * Maps (string) -> (struct uproc_database_metadata) */
    uproc_dict *metadata;
    /**
      * The forward matching ecurve.
      * \see uproc_ecurve
      */
    uproc_ecurve *fwd;
    /**
      * The backward matching ecurve.
      * \see uproc_ecurve
      */
    uproc_ecurve *rev;

    /**
     * The mapping of numerical ID to string ID.
     */
    uproc_idmap *idmaps[UPROC_RANKS_MAX];

    /**
     * Matrices containing protein thresholds.
     */
    uproc_matrix *prot_thresh_e2, *prot_thresh_e3;

    uproc_substmat *substmat;
};

// Formats into "t key: value" where t is a one-character type identifier
int metadata_format(char *buf, const void *key, const void *value, void *opaque)
{
    uproc_assert(strlen(key) < UPROC_DICT_KEY_SIZE_MAX);
    (void) opaque;
    const struct uproc_database_metadata *v = value;
    int bytes_left = UPROC_DICT_STORE_BUFFER_SIZE;
    int res = snprintf(buf, bytes_left, "%c %s: ", v->type, (const char *)key);

    if (res < 0 || res > bytes_left) {
        return -1;
    }
    bytes_left -= res;
    buf += res;

    switch (v->type) {
        case STR:
            res = snprintf(buf, bytes_left, "%s", v->value.str);
            break;
        case UINT:
            res = snprintf(buf, bytes_left, "%ju", v->value.uint);
            break;
        default:
            uproc_assert_msg(0, "metadata value has invalid type");
            return -1;
    }
    return (res > 0 && res <= bytes_left) ? 0 : -1;
}

int metadata_scan(const char *s, void *key, void *value, void *opaque)
{
    (void) opaque;
    struct uproc_database_metadata *v = value;

    // determine type by looking at the first character
    switch (*s) {
        case STR:
        case UINT:
            break;
        default:
            return uproc_error_msg(UPROC_EINVAL, "invalid type identifier %c",
                                   *s);
    }
    v->type = *s;
    s += 2;

    memset(key, 0, UPROC_DICT_KEY_SIZE_MAX);
    const char *sep = strchr(s, ':');
    if (!sep) {
        return -1;
    }
    strncpy(key, s, sep - s);

    s = sep + 2;  // skip ": "

    switch (v->type) {
        case STR:
            strncpy(v->value.str, s, sizeof v->value.str);
            break;
        case UINT:
            sscanf(s, "%ju", &v->value.uint);
    }
    return 0;
}

uproc_database *uproc_database_create(void)
{
    uproc_database *db = malloc(sizeof *db);
    if (!db) {
        uproc_error_msg(UPROC_ENOMEM,
                        "can not allocate memory for database object");
        return NULL;
    }
    *db = (struct uproc_database_s){0};
    db->metadata = uproc_dict_create(0, sizeof(struct uproc_database_metadata));
    if (!db->metadata) {
        free(db);
        return NULL;
    }
    return db;
}

struct db_progress_arg
{
    void (*parent_func)(double, void *);
    void *parent_arg;
    bool fwd_finished;
};

void db_progress(double percent, void *progress_arg)
{
    struct db_progress_arg *arg = progress_arg;
    percent /= 2;
    if (arg->fwd_finished) {
        percent += 50.0;
    }
    if (arg->parent_func) {
        arg->parent_func(percent, arg->parent_arg);
    }
}

uproc_database *uproc_database_load(const char *path, int version,
                                    void(*progress)(double, void*), void*progress_arg)
{
    return uproc_database_load_some(path, version, UPROC_DATABASE_LOAD_ALL, progress, progress_arg);
}

uproc_database *uproc_database_load_some(const char *path, int version, int which,
                                         void (*progress)(double, void*), void*progress_arg)
{
    uproc_database *db = uproc_database_create();
    if (!db) {
        return NULL;
    }

    if (version >= UPROC_DATABASE_V2) {
        uproc_dict *new_metadata = uproc_dict_load(
            0, sizeof(struct uproc_database_metadata), metadata_scan, NULL,
            UPROC_IO_GZIP, "%s/metadata", path);
        if (new_metadata) {
            uproc_dict_destroy(db->metadata);
            db->metadata = new_metadata;
        }
    } else {
        uproc_database_metadata_set_uint(db, "ranks", 1);
    }

    if (which & UPROC_DATABASE_LOAD_PROT_THRESH) {
        db->prot_thresh_e2 =
            uproc_matrix_load(UPROC_IO_GZIP, "%s/prot_thresh_e2", path);
        if (!db->prot_thresh_e2) {
            goto error;
        }
        db->prot_thresh_e3 =
            uproc_matrix_load(UPROC_IO_GZIP, "%s/prot_thresh_e3", path);
        if (!db->prot_thresh_e2) {
            goto error;
        }
    }

    if (which & UPROC_DATABASE_LOAD_IDMAPS) {
        if (version == UPROC_DATABASE_V1) {
            db->idmaps[0] =
                uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap", path);
        } else {
            uintmax_t ranks;
            int res = uproc_database_metadata_get_uint(db, "ranks", &ranks);
            if (res) {
                goto error;
            }
            for (uintmax_t i = 0; i < ranks; i++) {
                db->idmaps[i] = uproc_idmap_load(UPROC_IO_GZIP,
                                                 "%s/rank%ju.idmap", path, i);
                if (!db->idmaps[i]) {
                    goto error;
                }
            }
        }
    }

    if (version >= UPROC_DATABASE_V2 &&
        (which & UPROC_DATABASE_LOAD_SUBSTMAT)) {
        db->substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", path);
    }

    if (which & UPROC_DATABASE_LOAD_ECURVES) {
        struct db_progress_arg p_arg = {
            .parent_func = progress,
            .parent_arg = progress_arg,
            .fwd_finished = false,
        };
        db->fwd = uproc_ecurve_loadp(UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                     db_progress, &p_arg,
                                    "%s/fwd.ecurve", path);
        if (!db->fwd) {
            goto error;
        }
        p_arg.fwd_finished = true;
        db->rev = uproc_ecurve_loadp(UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                     db_progress, &p_arg,
                                    "%s/rev.ecurve", path);
        if (!db->rev) {
            goto error;
        }
    } else {
        progress(100.0, progress_arg);
    }

    return db;
error:
    uproc_database_destroy(db);
    return NULL;
}

int uproc_database_store(const uproc_database *db, const char *path,
                         int version, void (*progress)(double, void *),
                         void *progress_arg)
{
    uproc_assert(db);
    uproc_assert(db->metadata);

    if (version >= UPROC_DATABASE_V2) {
        if (uproc_dict_store(db->metadata, metadata_format, NULL, UPROC_IO_GZIP,
                             "%s/metadata", path)) {
            return -1;
        }
        for (int i = 0; i < UPROC_RANKS_MAX; i++) {
            if (!db->idmaps[i]) {
                continue;
            }
            if (uproc_idmap_store(db->idmaps[i], UPROC_IO_GZIP,
                                  "%s/rank%d.idmap", path, i)) {
                return -1;
            }
        }
    } else {
        if (uproc_idmap_store(db->idmaps[0], UPROC_IO_GZIP,
                              "%s/idmap", path)) {
            return -1;
        }
    }

    if (db->prot_thresh_e2) {
        if (uproc_matrix_store(db->prot_thresh_e2, UPROC_IO_GZIP,
                               "%s/prot_thresh_e2", path)) {
            return -1;
        }
    }
    if (db->prot_thresh_e3) {
        if (uproc_matrix_store(db->prot_thresh_e3, UPROC_IO_GZIP,
                               "%s/prot_thresh_e3", path)) {
            return -1;
        }
    }

    if (db->substmat) {
        if (uproc_substmat_store(db->substmat, UPROC_IO_GZIP,
                                 "%s/substmat", path)) {
            return -1;
        }
    }

    struct db_progress_arg p_arg = {
        .parent_func = progress,
        .parent_arg = progress_arg,
        .fwd_finished = false,
    };
    if (db->fwd) {
        if (uproc_ecurve_storep(db->fwd, UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                db_progress, &p_arg, "%s/fwd.ecurve", path)) {
            return -1;
        }
    }
    p_arg.fwd_finished = false;
    if (db->rev) {
        if (uproc_ecurve_storep(db->rev, UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                db_progress, &p_arg, "%s/rev.ecurve", path)) {
            return -1;
        }
    }
    progress(100.0, progress_arg);

    return 0;
}

uproc_database *uproc_database_unmarshal(uproc_io_stream *stream,
                                         int version,
                                         void (*progress)(double, void *),
                                         void *progress_arg)
{
    uproc_database *db = uproc_database_create();
    if (!db) {
        return NULL;
    }

    if (version >= UPROC_DATABASE_V2) {
        uproc_dict_destroy(db->metadata);
        db->metadata =
            uproc_dict_loads(0, sizeof(struct uproc_database_metadata),
                             metadata_scan, NULL, stream);
        db->substmat = uproc_substmat_loads(stream);
        if (!db->substmat) {
            goto error;
        }
        uintmax_t ranks_count = 1;
        int res = uproc_database_metadata_get_uint(db, "ranks", &ranks_count);
        if (res) {
            goto error;
        }
        for (uintmax_t i = 0; i < ranks_count; i++) {
            db->idmaps[i] = uproc_idmap_loads(stream);
            if (!db->idmaps[i]) {
                goto error;
            }
        }
    }

    if (!db->metadata) {
        goto error;
    }
    db->prot_thresh_e2 = uproc_matrix_loads(stream);
    if (!db->prot_thresh_e2) {
        goto error;
    }
    db->prot_thresh_e3 = uproc_matrix_loads(stream);
    if (!db->prot_thresh_e3) {
        goto error;
    }

    // in v1, idmap is stored after protein thresholds
    if (version == UPROC_DATABASE_V1) {
        db->idmaps[0] = uproc_idmap_loads(stream);
        if (!db->idmaps[0]) {
            goto error;
        }
    }

    struct db_progress_arg p_arg = {
        .parent_func = progress,
        .parent_arg = progress_arg,
        .fwd_finished = false,
    };
    db->fwd =
        uproc_ecurve_loadps(UPROC_ECURVE_PLAIN, db_progress, &p_arg, stream);
    if (!db->fwd) {
        goto error;
    }
    p_arg.fwd_finished = true;
    db->rev =
        uproc_ecurve_loadps(UPROC_ECURVE_PLAIN, db_progress, &p_arg, stream);
    if (!db->rev) {
        goto error;
    }

    return db;
error:
    uproc_database_destroy(db);
    return NULL;
}

int uproc_database_marshal(const uproc_database *db, uproc_io_stream *stream,
                           int version, void (*progress)(double, void *),
                           void *progress_arg)
{
    uproc_assert(db);
    int res = 0;
    if (version >= UPROC_DATABASE_V2) {
        res = uproc_dict_stores(db->metadata, metadata_format, NULL, stream);
        if (res) {
            goto error;
        }
        uintmax_t ranks_count = 1;
        res = uproc_database_metadata_get_uint(db, "ranks", &ranks_count);
        for (uintmax_t i = 0; !res && i < ranks_count; i++) {
            res = uproc_idmap_stores(db->idmaps[i], stream);
        }
        if (res) {
            goto error;
        }
    }

    res = uproc_matrix_stores(db->prot_thresh_e2, stream);
    if (res) {
        goto error;
    }
    res = uproc_matrix_stores(db->prot_thresh_e3, stream);
    if (res) {
        goto error;
    }

    // in v1, idmap is stored after protein thresholds
    if (version == UPROC_DATABASE_V1) {
        res = uproc_idmap_stores(db->idmaps[0], stream);
        if (res) {
            goto error;
        }
    }

    struct db_progress_arg p_arg = {
        .parent_func = progress,
        .parent_arg = progress_arg,
        .fwd_finished = false,
    };
    res = uproc_ecurve_storeps(db->fwd, UPROC_ECURVE_PLAIN, db_progress, &p_arg,
                               stream);
    if (res) {
        goto error;
    }
    p_arg.fwd_finished = true;
    res = uproc_ecurve_storeps(db->rev, UPROC_ECURVE_PLAIN, db_progress, &p_arg,
                               stream);
    if (res) {
        goto error;
    }

error:
    return res;
}

void uproc_database_destroy(uproc_database *db)
{
    if (!db) {
        return;
    }
    for (unsigned i = 0; i < UPROC_RANKS_MAX; i++) {
        uproc_idmap_destroy(db->idmaps[i]);
    }
    uproc_ecurve_destroy(db->fwd);
    uproc_ecurve_destroy(db->rev);
    uproc_matrix_destroy(db->prot_thresh_e2);
    uproc_matrix_destroy(db->prot_thresh_e3);
    uproc_dict_destroy(db->metadata);
    free(db);
}

uproc_ecurve *uproc_database_ecurve(uproc_database *db,
                                    enum uproc_ecurve_direction dir)
{
    uproc_assert(db);
    return dir == UPROC_ECURVE_FWD ? db->fwd : db->rev;
}

uproc_ecurve *uproc_database_ecurve_mv(uproc_database *db,
                                       enum uproc_ecurve_direction dir)
{
    uproc_assert(db);
    uproc_ecurve *ec;
    if (dir == UPROC_ECURVE_FWD) {
        ec = db->fwd;
        db->fwd = NULL;
    } else {
        ec = db->rev;
        db->rev = NULL;
    }
    return ec;
}

void uproc_database_set_ecurve(uproc_database *db,
                               enum uproc_ecurve_direction dir,
                               uproc_ecurve *ecurve)
{
    uproc_assert(db);
    if (dir == UPROC_ECURVE_FWD) {
        uproc_ecurve_destroy(db->fwd);
        db->fwd = ecurve;
    } else {
        uproc_ecurve_destroy(db->rev);
        db->rev = ecurve;
    }
}

uproc_idmap *uproc_database_idmap(uproc_database *db, uproc_rank rank)
{
    uproc_assert(db);
    return db->idmaps[rank];
}

void uproc_database_set_idmap(uproc_database *db, uproc_rank rank,
                              uproc_idmap *idmap)
{
    uproc_assert(db);
    uproc_idmap_destroy(db->idmaps[rank]);
    db->idmaps[rank] = idmap;
}

uproc_matrix *uproc_database_protein_threshold(uproc_database *db, int level)
{
    uproc_assert(db);
    switch (level) {
        case 0:
            return NULL;
        case 2:
            return db->prot_thresh_e2;
        case 3:
            return db->prot_thresh_e3;
    }
    uproc_error_msg(UPROC_EINVAL, "invalid protein threshold level %d", level);
    return NULL;
}

int uproc_database_set_protein_threshold(uproc_database *db, int level,
                                         uproc_matrix *prot_thresh)
{
    uproc_assert(db);
    uproc_assert(prot_thresh);
    switch (level) {
        case 2:
            uproc_matrix_destroy(db->prot_thresh_e2);
            db->prot_thresh_e2 = prot_thresh;
            break;
        case 3:
            uproc_matrix_destroy(db->prot_thresh_e3);
            db->prot_thresh_e3 = prot_thresh;
            break;
        default:
            return uproc_error_msg(UPROC_EINVAL,
                                   "invalid protein threshold level %d", level);
    }
    return 0;
}

uproc_alphabet *uproc_database_alphabet(uproc_database *db)
{
    uproc_assert(db);
    uproc_assert(db->fwd || db->rev);
    if (db->fwd) {
        return uproc_ecurve_alphabet(db->fwd);
    }
    return uproc_ecurve_alphabet(db->rev);
}

void uproc_database_set_substitution_matrix(uproc_database *db,
                                            uproc_substmat *substmat)
{
    uproc_assert(db);
    uproc_assert(substmat);
    uproc_substmat_destroy(db->substmat);
    db->substmat = substmat;
}

uproc_substmat *uproc_database_substitution_matrix(uproc_database *db)
{
    if (!db) {
        uproc_error_msg(UPROC_EINVAL, "db parameter must not be NULL");
        return NULL;
    }
    return db->substmat;
}

int metadata_get(const uproc_database *db, const char *key,
                 struct uproc_database_metadata *value)
{
    uproc_assert(db);
    uproc_assert(db->metadata);
    if (uproc_dict_get(db->metadata, key, value)) {
        return -1;
    }
    return 0;
}

int uproc_database_metadata_get_uint(const uproc_database *db, const char *key,
                                     uintmax_t *value)
{
    struct uproc_database_metadata v;
    if (metadata_get(db, key, &v)) {
        return -1;
    }
    uproc_assert(v.type == UINT);
    *value = v.value.uint;
    return 0;
}

int uproc_database_metadata_get_str(
    const uproc_database *db, const char *key,
    char value[static UPROC_DATABASE_METADATA_STR_SIZE])
{
    struct uproc_database_metadata v;
    if (metadata_get(db, key, &v)) {
        return -1;
    }
    uproc_assert(v.type == STR);
    strncpy(value, v.value.str, sizeof v.value.str);
    return 0;
}

int metadata_set(uproc_database *db, const char *key,
                 const struct uproc_database_metadata *value)
{
    uproc_assert(db);
    uproc_assert(db->metadata);
    if (uproc_dict_set(db->metadata, key, value)) {
        return -1;
    }
    return 0;
}

int uproc_database_metadata_set_uint(uproc_database *db, const char *key,
                                     uintmax_t value)
{
    struct uproc_database_metadata v;
    v.type = UINT;
    v.value.uint = value;
    return metadata_set(db, key, &v);
}

int uproc_database_metadata_set_str(uproc_database *db, const char *key,
                                    char *value)
{
    struct uproc_database_metadata v;
    v.type = STR;
    v.value.str[sizeof v.value.str - 1] = '\0';
    strncpy(v.value.str, value, sizeof v.value.str - 1);
    char *p = strchr(v.value.str, '\n');
    if (p) {
        *p = '\0';
    }
    return metadata_set(db, key, &v);
}
