/* Map string identifier to family number
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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
#include <string.h>
#include <stdarg.h>
#include "uproc/idmap.h"
#include "uproc/io.h"
#include "uproc/error.h"

struct uproc_idmap_s
{
    uproc_family n;
    char *s[UPROC_FAMILY_MAX];
};

uproc_idmap *uproc_idmap_create(void)
{
    uproc_idmap *map = malloc(sizeof *map);
    if (!map) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *map = (struct uproc_idmap_s){0, {NULL}};
    return map;
}

void uproc_idmap_destroy(uproc_idmap *map)
{
    if (!map) {
        return;
    }
    uproc_family i;
    for (i = 0; i < map->n; i++) {
        free(map->s[i]);
    }
    free(map);
}

uproc_family uproc_idmap_family(uproc_idmap *map, const char *s)
{
    uproc_family i;
    for (i = 0; i < map->n; i++) {
        if (!strcmp(s, map->s[i])) {
            return i;
        }
    }
    if (map->n == UPROC_FAMILY_MAX) {
        uproc_error_msg(UPROC_ENOENT, "idmap exhausted");
        return UPROC_FAMILY_INVALID;
    }
    size_t len = strlen(s);
    map->s[map->n] = malloc(len + 1);
    if (!map->s[map->n]) {
        return uproc_error(UPROC_ENOMEM);
    }
    memcpy(map->s[map->n], s, len + 1);
    map->n += 1;
    return i;
}

char *uproc_idmap_str(const uproc_idmap *map, uproc_family family)
{
    return map->s[family];
}

uproc_idmap *uproc_idmap_loads(uproc_io_stream *stream)
{
    int res;
    struct uproc_idmap_s *map;
    uproc_family i;
    unsigned long n;
    char *line = NULL;
    size_t sz;

    map = uproc_idmap_create();
    if (!map) {
        return NULL;
    }
    if (!uproc_io_getline(&line, &sz, stream)) {
        goto error;
    }
    res = sscanf(line, "[%lu]\n", &n);
    if (res != 1) {
        uproc_error_msg(UPROC_EINVAL, "invalid idmap header");
        goto error;
    }
    if (n > UPROC_FAMILY_MAX) {
        uproc_error_msg(UPROC_EINVAL, "idmap size too large");
        goto error;
    }
    for (i = 0; i < n; i++) {
        size_t len;
        if (!uproc_io_getline(&line, &sz, stream)) {
            if (uproc_io_eof(stream)) {
                uproc_error_msg(UPROC_EINVAL, "unexpected end of file");
            }
            goto error;
        }
        len = strlen(line);
        if (len < 1 || line[len - 1] != '\n') {
            uproc_error_msg(UPROC_EINVAL, "line %" UPROC_FAMILY_PRI
                                          ": expected newline after ID",
                            i + 2);
            goto error;
        }
        line[len - 1] = '\0';
        if (uproc_idmap_family(map, line) != i) {
            uproc_error_msg(UPROC_EINVAL,
                            "line %" UPROC_FAMILY_PRI ": duplicate ID", i + 2);
            goto error;
        }
    }

    if (0) {
    error:
        uproc_idmap_destroy(map);
        map = NULL;
    }
    free(line);
    return map;
}

uproc_idmap *uproc_idmap_loadv(enum uproc_io_type iotype, const char *pathfmt,
                               va_list ap)
{
    struct uproc_idmap_s *map;
    uproc_io_stream *stream = uproc_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return NULL;
    }
    map = uproc_idmap_loads(stream);
    (void)uproc_io_close(stream);
    return map;
}

uproc_idmap *uproc_idmap_load(enum uproc_io_type iotype, const char *pathfmt,
                              ...)
{
    struct uproc_idmap_s *map;
    va_list ap;
    va_start(ap, pathfmt);
    map = uproc_idmap_loadv(iotype, pathfmt, ap);
    va_end(ap);
    return map;
}

int uproc_idmap_stores(const uproc_idmap *map, uproc_io_stream *stream)
{
    uproc_family i;
    uproc_io_printf(stream, "[%" UPROC_FAMILY_PRI "]\n", map->n);
    for (i = 0; i < map->n; i++) {
        if (uproc_io_printf(stream, "%s\n", map->s[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

int uproc_idmap_storev(const uproc_idmap *map, enum uproc_io_type iotype,
                       const char *pathfmt, va_list ap)
{
    int res;
    uproc_io_stream *stream = uproc_io_openv("w", iotype, pathfmt, ap);
    if (!stream) {
        return -1;
    }
    res = uproc_idmap_stores(map, stream);
    uproc_io_close(stream);
    return res;
}

int uproc_idmap_store(const uproc_idmap *map, enum uproc_io_type iotype,
                      const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_idmap_storev(map, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}
