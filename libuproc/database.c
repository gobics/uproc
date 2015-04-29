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
    uproc_idmap *idmap;

    /**
     * Matrices containing protein thresholds.
     */
    uproc_matrix *prot_thresh_e2, *prot_thresh_e3;
};

// Formats into "t key: value" where t is a one-character type identifier
int metadata_format(char *buf, const void *key, const void *value, void *opaque)
{
    uproc_assert(strlen(key) < UPROC_DICT_KEY_SIZE_MAX);
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

uproc_database *uproc_database_load(const char *path)
{
    uproc_database *db = uproc_database_create();
    if (!db) {
        return NULL;
    }
    uproc_dict_destroy(db->metadata);
    db->metadata = uproc_dict_load(0, sizeof(struct uproc_database_metadata),
                                   metadata_scan, NULL, UPROC_IO_GZIP,
                                   "%s/metadata", path);
    if (!db->metadata) {
        // We'll allow old databases without metadata (for now)
        // goto error;
    }

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

    db->idmap = uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap", path);
    if (!db->idmap) {
        goto error;
    }
    db->fwd = uproc_ecurve_load(UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                "%s/fwd.ecurve", path);
    if (!db->fwd) {
        goto error;
    }
    db->rev = uproc_ecurve_load(UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                "%s/rev.ecurve", path);
    if (!db->rev) {
        goto error;
    }

    return db;
error:
    uproc_database_destroy(db);
    return NULL;
}

int uproc_database_store(const uproc_database *db, const char *path,
                         void (*progress)(double))
{
    uproc_assert(db);
    uproc_assert(db->metadata && db->idmap);

    if (uproc_dict_store(db->metadata, metadata_format, NULL, UPROC_IO_GZIP,
                         "%s/metadata", path)) {
        return -1;
    }
    if (uproc_idmap_store(db->idmap, UPROC_IO_GZIP, "%s/idmap", path)) {
        return -1;
    }

    if (db->fwd) {
        if (uproc_ecurve_storep(db->fwd, UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                progress, "%s/fwd.ecurve", path)) {
            return -1;
        }
    }
    if (db->rev) {
        if (uproc_ecurve_storep(db->rev, UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                                progress, "%s/rev.ecurve", path)) {
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

    return 0;
}

void uproc_database_destroy(uproc_database *db)
{
    if (!db) {
        return;
    }
    uproc_dict_destroy(db->metadata);
    uproc_ecurve_destroy(db->fwd);
    uproc_ecurve_destroy(db->rev);
    uproc_idmap_destroy(db->idmap);
    uproc_matrix_destroy(db->prot_thresh_e2);
    uproc_matrix_destroy(db->prot_thresh_e3);
    free(db);
}

uproc_ecurve *uproc_database_ecurve(uproc_database *db,
                                    enum uproc_ecurve_direction dir)
{
    uproc_assert(db);
    return dir == UPROC_ECURVE_FWD ? db->fwd : db->rev;
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

uproc_idmap *uproc_database_idmap(uproc_database *db)
{
    uproc_assert(db);
    return db->idmap;
}

void uproc_database_set_idmap(uproc_database *db, uproc_idmap *idmap)
{
    uproc_assert(db);
    uproc_idmap_destroy(db->idmap);
    db->idmap = idmap;
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
    return metadata_set(db, key, &v);
}
