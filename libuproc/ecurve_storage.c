/* Load/store ecurves from/to files
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
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"
#include "uproc/io.h"
#include "uproc/list.h"
#include "uproc/word.h"

#include "ecurve_internal.h"

#define STR1(x) #x
#define STR(x) STR1(x)
#define BUFSZ 1024
#define COMMENT_CHAR '#'
#define HEADER_PRI \
    ">> alphabet: %." STR(UPROC_ALPHABET_SIZE) "s\n"
#define HEADER_SCN \
    ">> alphabet: %"  STR(UPROC_ALPHABET_SIZE) "c"

#define PREFIX_PRI ">%." STR(UPROC_PREFIX_LEN) "s\n"
#define PREFIX_SCN ">%"  STR(UPROC_PREFIX_LEN) "c"

#define SUFFIX_PRI "%." STR(UPROC_SUFFIX_LEN) "s %" UPROC_FAMILY_PRI "\n"
#define SUFFIX_SCN "%"  STR(UPROC_SUFFIX_LEN) "c %" UPROC_FAMILY_SCN

static int
read_line(uproc_io_stream *stream, char *line, size_t n)
{
    do {
        if (!uproc_io_gets(line, n, stream)) {
            return -1;
        }
    } while (line[0] == COMMENT_CHAR);
    return 0;
}

static int
parse_header(const char *line, char *alpha)
{
    int res;
    res = sscanf(line, HEADER_SCN, alpha);
    alpha[UPROC_ALPHABET_SIZE] = '\0';
    if (res != 1) {
        return uproc_error_msg(UPROC_EINVAL, "invalid header: \"%s\"", line);
    }
    return 0;
}

static int
parse_prefix(const char *line, const uproc_alphabet *alpha, uproc_prefix *pfx)
{
    int res;
    char tmp[UPROC_WORD_LEN + 1] = "";
    struct uproc_word word = UPROC_WORD_INITIALIZER;

    memset(tmp, 'A', sizeof tmp - 1);
    memcpy(tmp, line, UPROC_PREFIX_LEN);
    res = uproc_word_from_string(&word, tmp, alpha);
    if (res) {
        return res;
    }
    *pfx = word.prefix;
    return 0;

}

static int
parse_suffixentry(const char *line, const uproc_alphabet *alpha,
             struct uproc_ecurve_suffixentry *entry)
{
    int res;
    char tmp[UPROC_WORD_LEN + 1] = "";
    struct uproc_word word = UPROC_WORD_INITIALIZER;

    memset(tmp, 'A', sizeof tmp - 1);
    memcpy(tmp + UPROC_PREFIX_LEN, line, UPROC_SUFFIX_LEN);
    res = uproc_word_from_string(&word, tmp, alpha);
    if (res) {
        return res;
    }
    entry->suffix = word.suffix;

    line += UPROC_SUFFIX_LEN + 1;
    res = sscanf(line, "%" UPROC_FAMILY_SCN, &entry->family);
    if (res != 1) {
        return uproc_error_msg(UPROC_EINVAL,
                               "invalid family identifier: \"%s\"", line);
    }
    return 0;
}


static uproc_ecurve *
load_plain(uproc_io_stream *stream, void (*progress)(double))
{
    int res = 0;
    uproc_ecurve *ec = NULL;

    char line[BUFSZ];
    char alpha[UPROC_ALPHABET_SIZE + 1];
    uproc_alphabet *ec_alpha;

    uproc_prefix prefix = UPROC_PREFIX_MAX + 1;
    uproc_list *suffix_list;
    struct uproc_ecurve_suffixentry suffix_entry;

    suffix_list = uproc_list_create(sizeof suffix_entry);
    if (!suffix_list) {
        return NULL;
    }

    if (read_line(stream, line, sizeof line)) {
        goto error;
    }
    if (parse_header(line, alpha)) {
        goto error;
    }
    ec = uproc_ecurve_create(alpha, 0);
    if (!ec) {
        goto error;
        return NULL;
    }
    ec_alpha = uproc_ecurve_alphabet(ec);

    while (res = read_line(stream, line, sizeof line), !res) {
        if (line[0] == '>' || line[0] == '.') {
            if (uproc_list_size(suffix_list)) {
                res = uproc_ecurve_add_prefix(ec, prefix, suffix_list);
                if (res) {
                    goto error;
                }
                uproc_list_clear(suffix_list);
            }
            /* the end */
            if (line[0] == '.') {
                res = 0;
                break;
            }
            res = parse_prefix(line + 1, ec_alpha, &prefix);
            if (res) {
                goto error;
            }
            if (progress) {
                progress(100.0 / UPROC_PREFIX_MAX * prefix);
            }
        }
        else {
            res = parse_suffixentry(line, ec_alpha, &suffix_entry);
            if (res) {
                goto error;
            }
            res = uproc_list_append(suffix_list, &suffix_entry);
            if (res) {
                goto error;
            }
        }
    }
    uproc_ecurve_finalize(ec);
    if (progress) {
        progress(100.0);
    }
    if (res) {
error:
        uproc_ecurve_destroy(ec);
        ec = NULL;
    }
    uproc_list_destroy(suffix_list);
    return ec;
}

static int
store_header(uproc_io_stream *stream, const char *alpha)
{
    int res;
    res = uproc_io_printf(stream, HEADER_PRI, alpha);
    return (res > 0) ? 0 : -1;
}

static int
store_prefix(uproc_io_stream *stream, const uproc_alphabet *alpha,
             uproc_prefix prefix)
{
    int res;
    char str[UPROC_WORD_LEN + 1];
    struct uproc_word word = UPROC_WORD_INITIALIZER;
    word.prefix = prefix;
    res = uproc_word_to_string(str, &word, alpha);
    if (res) {
        return res;
    }
    str[UPROC_PREFIX_LEN] = '\0';
    res = uproc_io_printf(stream, PREFIX_PRI, str);
    return (res > 0) ? 0 : -1;
}

static int
store_suffix(uproc_io_stream *stream, const uproc_alphabet *alpha,
             uproc_suffix suffix, uproc_family family)
{
    int res;
    char str[UPROC_WORD_LEN + 1];
    struct uproc_word word = UPROC_WORD_INITIALIZER;
    word.suffix = suffix;
    res = uproc_word_to_string(str, &word, alpha);
    if (res) {
        return res;
    }
    res = uproc_io_printf(stream, SUFFIX_PRI, &str[UPROC_PREFIX_LEN], family);
    return (res > 0) ? 0 : -1;
}

static int
store_plain(const struct uproc_ecurve_s *ecurve, uproc_io_stream *stream,
            void (*progress)(double))
{
    int res;
    size_t p;

    res = store_header(stream, uproc_alphabet_str(ecurve->alphabet));

    if (res) {
        return res;
    }

    for (p = 0; p <= UPROC_PREFIX_MAX; p++) {
        pfxtab_count suffix_count = ecurve->prefixes[p].count;
        pfxtab_suffix first = ecurve->prefixes[p].first;

        if (!suffix_count || ECURVE_ISEDGE(ecurve->prefixes[p])) {
            continue;
        }

        res = store_prefix(stream, ecurve->alphabet, p);
        if (res) {
            return res;
        }

        for (pfxtab_count i = 0; i < suffix_count; i++) {
            res = store_suffix(stream, ecurve->alphabet,
                               ecurve->suffixes[first + i],
                               ecurve->families[first + i]);
            if (res) {
                return res;
            }
        }
        if (progress) {
            progress(100.0 / UPROC_PREFIX_MAX * p);
        }
    }
    uproc_io_printf(stream, ".\n");
    if (progress) {
        progress(100.0);
    }
    return 0;
}


#if !HAVE_MMAP
static uproc_ecurve *
load_binary(uproc_io_stream *stream, void (*progress)(double))
{
    struct uproc_ecurve_s *ecurve;
    size_t sz;
    size_t suffix_count;
    char alpha[UPROC_ALPHABET_SIZE + 1];

    sz = uproc_io_read(alpha, sizeof *alpha, UPROC_ALPHABET_SIZE, stream);
    if (sz != UPROC_ALPHABET_SIZE) {
        uproc_error(UPROC_ERRNO);
        return NULL;
    }
    alpha[UPROC_ALPHABET_SIZE] = '\0';

    sz = uproc_io_read(&suffix_count, sizeof suffix_count, 1, stream);
    if (sz != 1) {
        uproc_error(UPROC_ERRNO);
        return NULL;
    }

    ecurve = uproc_ecurve_create(alpha, suffix_count);
    if (!ecurve) {
        return NULL;
    }

    sz = uproc_io_read(ecurve->suffixes, sizeof *ecurve->suffixes,
                       suffix_count, stream);
    if (sz != suffix_count) {
        goto error;
    }
    sz = uproc_io_read(ecurve->families, sizeof *ecurve->families,
                       suffix_count, stream);
    if (sz != suffix_count) {
        goto error;
    }

    for (uproc_prefix i = 0; i <= UPROC_PREFIX_MAX; i++) {
        sz = uproc_io_read(&ecurve->prefixes[i].first,
                           sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            goto error;
        }
        sz = uproc_io_read(&ecurve->prefixes[i].count,
                           sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            goto error;
        }
        if (progress) {
            progress(100.0 / UPROC_PREFIX_MAX * i);
        }
    }
    if (progress) {
        progress(100.0);
    }
    return ecurve;
error:
    uproc_error(UPROC_ERRNO);
    uproc_ecurve_destroy(ecurve);
    return NULL;
}

static int
store_binary(const struct uproc_ecurve_s *ecurve, uproc_io_stream *stream,
             void (*progress)(double))
{
    size_t sz;
    sz = uproc_io_write(uproc_alphabet_str(ecurve->alphabet), 1,
                        UPROC_ALPHABET_SIZE, stream);
    if (sz != UPROC_ALPHABET_SIZE) {
        return uproc_error(UPROC_ERRNO);
    }

    sz = uproc_io_write(&ecurve->suffix_count, sizeof ecurve->suffix_count, 1,
                        stream);
    if (sz != 1) {
        return uproc_error(UPROC_ERRNO);
    }

    sz = uproc_io_write(ecurve->suffixes, sizeof *ecurve->suffixes,
                        ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        return uproc_error(UPROC_ERRNO);
    }
    sz = uproc_io_write(ecurve->families, sizeof *ecurve->families,
                        ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        return uproc_error(UPROC_ERRNO);
    }

    for (uproc_prefix i = 0; i <= UPROC_PREFIX_MAX; i++) {
        sz = uproc_io_write(&ecurve->prefixes[i].first,
                            sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            return uproc_error(UPROC_ERRNO);
        }
        sz = uproc_io_write(&ecurve->prefixes[i].count,
                            sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            return uproc_error(UPROC_ERRNO);
        }
        if (progress) {
            progress(100.0 / UPROC_PREFIX_MAX * i);
        }
    }
    if (progress) {
        progress(100.0);
    }
    return 0;
}
#endif


uproc_ecurve *
uproc_ecurve_loads(enum uproc_ecurve_format format, uproc_io_stream *stream)
{
    return uproc_ecurve_loadps(format, NULL, stream);
}


uproc_ecurve *
uproc_ecurve_loadps(enum uproc_ecurve_format format, void (*progress)(double),
                    uproc_io_stream *stream)
{
    if (format == UPROC_ECURVE_BINARY) {
#if HAVE_MMAP
        /* if HAVE_MMAP is defined, uproc_mmap_map is called instead */
        uproc_error_msg(UPROC_EINVAL, "can't load mmap format from stream");
        return NULL;
#else
        return load_binary(stream, progress);
#endif
    }
    return load_plain(stream, progress);
}


uproc_ecurve *
uproc_ecurve_loadv(enum uproc_ecurve_format format,
                   enum uproc_io_type iotype,
                   const char *pathfmt, va_list ap)
{
    return uproc_ecurve_loadpv(format, iotype, NULL, pathfmt, ap);
}


uproc_ecurve *
uproc_ecurve_loadpv(enum uproc_ecurve_format format,
                    enum uproc_io_type iotype, void (*progress)(double),
                    const char *pathfmt, va_list ap)
{
    struct uproc_ecurve_s *ec;
    va_list aq;
    uproc_io_stream *stream;
    const char *mode[] = {
        [UPROC_ECURVE_BINARY] = "rb",
        [UPROC_ECURVE_PLAIN] = "r",
    };

    va_copy(aq, ap);

#if HAVE_MMAP
    if (format == UPROC_ECURVE_BINARY) {
        ec = uproc_ecurve_mmapv(pathfmt, aq);
        if (ec && progress) {
            progress(100.0);
        }
        va_end(aq);
        return ec;
    }
#endif

    stream = uproc_io_openv(mode[format], iotype, pathfmt, aq);
    va_end(aq);
    if (!stream) {
        return NULL;
    }
    ec = uproc_ecurve_loadps(format, progress, stream);
    uproc_io_close(stream);
    return ec;
}

uproc_ecurve *
uproc_ecurve_load(enum uproc_ecurve_format format, enum uproc_io_type iotype,
                  const char *pathfmt, ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_loadv(format, iotype, pathfmt, ap);
    va_end(ap);
    return ec;
}


uproc_ecurve *
uproc_ecurve_loadp(enum uproc_ecurve_format format, enum uproc_io_type iotype,
                   void (*progress)(double), const char *pathfmt, ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_loadpv(format, iotype, progress, pathfmt, ap);
    va_end(ap);
    return ec;
}


int
uproc_ecurve_stores(const uproc_ecurve *ecurve,
                    enum uproc_ecurve_format format, uproc_io_stream *stream)
{
    return uproc_ecurve_storeps(ecurve, format, NULL, stream);
}


int
uproc_ecurve_storeps(const uproc_ecurve *ecurve,
                    enum uproc_ecurve_format format,
                    void (*progress)(double),
                    uproc_io_stream *stream)
{
    if (format == UPROC_ECURVE_BINARY) {
#if HAVE_MMAP
        return uproc_error_msg(UPROC_EINVAL,
                               "can't store mmap format to stream");
#else
        return store_binary(ecurve, stream, progress);
#endif
    }
    return store_plain(ecurve, stream, progress);
}


int
uproc_ecurve_storev(const uproc_ecurve *ecurve,
                    enum uproc_ecurve_format format, enum uproc_io_type iotype,
                    const char *pathfmt, va_list ap)
{
    return uproc_ecurve_storepv(ecurve, format, iotype, NULL, pathfmt, ap);
}


int
uproc_ecurve_storepv(const uproc_ecurve *ecurve,
                     enum uproc_ecurve_format format,
                     enum uproc_io_type iotype,
                     void (*progress)(double),
                     const char *pathfmt, va_list ap)
{
    int res;
    va_list aq;
    uproc_io_stream *stream;
    static const char *mode[] = {
        [UPROC_ECURVE_BINARY] = "wb",
        [UPROC_ECURVE_PLAIN] = "w",
    };

    va_copy(aq, ap);

#if HAVE_MMAP
    if (format == UPROC_ECURVE_BINARY) {
        res = uproc_ecurve_mmap_storev(ecurve, pathfmt, aq);
        va_end(aq);
        if (!res && progress) {
            progress(100.0);
        }
        return res;
    }
#endif

    stream = uproc_io_openv(mode[format], iotype, pathfmt, aq);
    if (!stream) {
        return -1;
    }
    res = uproc_ecurve_storeps(ecurve, format, progress, stream);
    uproc_io_close(stream);
    return res;
}


int
uproc_ecurve_store(const uproc_ecurve *ecurve, enum uproc_ecurve_format format,
                   enum uproc_io_type iotype, const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_storev(ecurve, format, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}


int
uproc_ecurve_storep(const uproc_ecurve *ecurve, enum uproc_ecurve_format format,
                   enum uproc_io_type iotype, void (*progress)(double),
                   const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_storepv(ecurve, format, iotype, progress, pathfmt, ap);
    va_end(ap);
    return res;
}
