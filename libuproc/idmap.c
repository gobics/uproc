/* Map string identifier to family number
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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

int
uproc_idmap_init(struct uproc_idmap *map)
{
    *map = (struct uproc_idmap) UPROC_IDMAP_INITIALIZER;
    return 0;
}

void
uproc_idmap_destroy(struct uproc_idmap *map)
{
    uproc_family i;
    for (i = 0; i < map->n; i++) {
        free(map->s[i]);
        map->s[i] = NULL;
    }
}

uproc_family
uproc_idmap_family(struct uproc_idmap *map, const char *s)
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

const char *
uproc_idmap_str(const struct uproc_idmap *map, uproc_family family)
{
    return map->s[family];
}

int
uproc_idmap_loads(struct uproc_idmap *map, uproc_io_stream *stream)
{
    int res;
    uproc_family i;
    unsigned long long n;
    char buf[UPROC_FAMILY_MAX + 1];

    res = uproc_idmap_init(map);
    if (res) {
        return res;
    }
    if (!uproc_io_gets(buf, sizeof buf, stream)) {
        return -1;
    }
    res = sscanf(buf, "[%llu]\n", &n);
    if (res != 1) {
        return uproc_error_msg(UPROC_EINVAL, "invalid idmap header");
    }
    if (n > UPROC_FAMILY_MAX) {
        return uproc_error_msg(UPROC_EINVAL, "idmap size too large");
    }
    for (i = 0; i < n; i++) {
        size_t len;
        if (!uproc_io_gets(buf, sizeof buf, stream)) {
            return -1;
        }
        len = strlen(buf);
        if (len < 1 || buf[len - 1] != '\n') {
            return uproc_error_msg(UPROC_EINVAL,
                                   "line %u: expected newline after ID",
                                   (unsigned)i + 2);
        }
        buf[len - 1] = '\0';
        if (uproc_idmap_family(map, buf) != i) {
            return uproc_error_msg(UPROC_EINVAL,
                                   "line %u: duplicate ID",
                                   (unsigned)i + 2);
        }
    }
    return 0;
}

int
uproc_idmap_loadv(struct uproc_idmap *map, enum uproc_io_type iotype,
                  const char *pathfmt, va_list ap)
{
    int res;
    uproc_io_stream *stream = uproc_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return -1;
    }
    res = uproc_idmap_loads(map, stream);
    (void) uproc_io_close(stream);
    return res;
}

int
uproc_idmap_load(struct uproc_idmap *map, enum uproc_io_type iotype,
                 const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_idmap_loadv(map, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

int
uproc_idmap_stores(const struct uproc_idmap *map, uproc_io_stream *stream)
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


int
uproc_idmap_storev(const struct uproc_idmap *map, enum uproc_io_type iotype,
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

int
uproc_idmap_store(const struct uproc_idmap *map, enum uproc_io_type iotype,
                  const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_idmap_storev(map, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}
