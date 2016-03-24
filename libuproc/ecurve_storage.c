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

static int read_line(uproc_io_stream *stream, char *line, size_t n)
{
    do {
        if (!uproc_io_gets(line, n, stream)) {
            return -1;
        }
    } while (line[0] == COMMENT_CHAR);
    return 0;
}

static int parse_header(const char *line, char *alpha, uproc_rank *ranks_count)
{
    int res;
    // old files are missing the "ranks: " part and have always 1 class.
    *ranks_count = 1;
    res = sscanf(line,
                 ">> alphabet: %" STR(UPROC_ALPHABET_SIZE) "c "
                 "ranks: %hhu", alpha,
                 ranks_count);
    alpha[UPROC_ALPHABET_SIZE] = '\0';
    if (res < 1) {
        return uproc_error_msg(UPROC_EINVAL, "invalid header: \"%s\"", line);
    }
    return 0;
}

static int parse_prefix(const char *line, const uproc_alphabet *alpha,
                        uproc_prefix *pfx)
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

static int parse_suffixentry(const char *line, const uproc_alphabet *alpha,
                             uproc_rank ranks_count,
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
    for (int i = 0; *line != '\n' && i < ranks_count; i++) {
        int consumed;
        res = sscanf(line, " %" UPROC_CLASS_SCN "%n", &entry->classes[i],
                     &consumed);
        if (res != 1) {
            return uproc_error_msg(UPROC_EINVAL,
                                   "invalid class identifier: \"%s\"", line);
        }
        line += consumed;
    }
    if (*line != '\n') {
        return uproc_error_msg(UPROC_EINVAL, "too many class identifiers");
    }
    return 0;
}

static uproc_ecurve *load_plain(uproc_io_stream *stream,
                                void (*progress)(double, void *),
                                void *progress_arg)
{
    int res = 0;
    uproc_ecurve *ec = NULL;

    char line[BUFSZ];
    char alpha[UPROC_ALPHABET_SIZE + 1];
    uproc_alphabet *ec_alpha;
    uproc_rank ranks_count;

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
    if (parse_header(line, alpha, &ranks_count)) {
        goto error;
    }
    ec = uproc_ecurve_create(alpha, ranks_count, 0);
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
                progress(100.0 / UPROC_PREFIX_MAX * prefix, progress_arg);
            }
        } else {
            res = parse_suffixentry(line, ec_alpha, ec->ranks_count,
                                    &suffix_entry);
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
        progress(100.0, progress_arg);
    }
    if (res) {
    error:
        uproc_ecurve_destroy(ec);
        ec = NULL;
    }
    uproc_list_destroy(suffix_list);
    return ec;
}

static int store_prefix(uproc_io_stream *stream, const uproc_alphabet *alpha,
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
    res = uproc_io_printf(stream, ">%s\n", str);
    return (res > 0) ? 0 : -1;
}

static int store_suffix(uproc_io_stream *stream, const uproc_alphabet *alpha,
                        uproc_suffix suffix, uproc_rank ranks_count,
                        const uproc_class classes[UPROC_RANKS_MAX])
{
    int res;
    char str[UPROC_WORD_LEN + 1];
    struct uproc_word word = UPROC_WORD_INITIALIZER;
    word.suffix = suffix;
    res = uproc_word_to_string(str, &word, alpha);
    if (res) {
        return res;
    }
    res = uproc_io_printf(stream, "%s", str + UPROC_PREFIX_LEN);
    for (int i = 0; res >= 0 && i < ranks_count; i++) {
        res = uproc_io_printf(stream, " %" UPROC_CLASS_PRI, classes[i]);
    }
    if (res < 0) {
        return -1;
    }
    return uproc_io_putc('\n', stream) == '\n' ? 0 : -1;
}

static int store_plain(const struct uproc_ecurve_s *ecurve,
                       uproc_io_stream *stream,
                       void (*progress)(double, void *), void *progress_arg)
{
    int res;
    size_t p;

    res = uproc_io_printf(stream, ">> alphabet: %s ranks: %u\n",
                          uproc_alphabet_str(ecurve->alphabet),
                          ecurve->ranks_count);

    if (res < 0) {
        return -1;
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
            res = store_suffix(
                stream, ecurve->alphabet, ecurve->suffixes[first + i],
                ecurve->ranks_count,
                &ecurve->classes[(first + i) * ecurve->ranks_count]);
            if (res) {
                return res;
            }
        }
        if (progress) {
            progress(100.0 / UPROC_PREFIX_MAX * p, progress_arg);
        }
    }
    uproc_io_printf(stream, ".\n");
    if (progress) {
        progress(100.0, progress_arg);
    }
    return 0;
}

#if !(HAVE_MMAP && USE_MMAP)
static uproc_ecurve *load_binary(uproc_io_stream *stream,
                                 void (*progress)(double, void *),
                                 void *progress_arg)
{
    size_t sz;
    char alpha[UPROC_ALPHABET_SIZE + 1];
    sz = uproc_io_read(alpha, sizeof *alpha, UPROC_ALPHABET_SIZE, stream);
    if (sz != UPROC_ALPHABET_SIZE) {
        uproc_error(UPROC_ERRNO);
        return NULL;
    }
    alpha[UPROC_ALPHABET_SIZE] = '\0';

    uproc_rank ranks_count;
    sz = uproc_io_read(&ranks_count, sizeof ranks_count, 1, stream);
    if (sz != 1) {
        uproc_error(UPROC_ERRNO);
        return NULL;
    }

    size_t suffix_count;
    sz = uproc_io_read(&suffix_count, sizeof suffix_count, 1, stream);
    if (sz != 1) {
        uproc_error(UPROC_ERRNO);
        return NULL;
    }

    struct uproc_ecurve_s *ecurve =
        uproc_ecurve_create(alpha, ranks_count, suffix_count);
    if (!ecurve) {
        return NULL;
    }

    if (progress) {
        progress(0.1, progress);
    }

    sz = uproc_io_read(ecurve->suffixes, sizeof *ecurve->suffixes, suffix_count,
                       stream);
    if (sz != suffix_count) {
        goto error;
    }
    if (progress) {
        progress(25.0, progress);
    }
    sz = uproc_io_read(ecurve->classes, sizeof *ecurve->classes,
                       ranks_count * suffix_count, stream);
    if (sz != ranks_count * suffix_count) {
        goto error;
    }
    if (progress) {
        progress(50.0, progress);
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
            progress(50.0 + 50.0 / UPROC_PREFIX_MAX * i, progress);
        }
    }
    if (progress) {
        progress(100.0, progress);
    }
    return ecurve;
error:
    uproc_error(UPROC_ERRNO);
    uproc_ecurve_destroy(ecurve);
    return NULL;
}

static int store_binary(const struct uproc_ecurve_s *ecurve,
                        uproc_io_stream *stream,
                        void (*progress)(double, void *), void *progress_arg)
{
    size_t sz;
    sz = uproc_io_write(uproc_alphabet_str(ecurve->alphabet), 1,
                        UPROC_ALPHABET_SIZE, stream);
    if (sz != UPROC_ALPHABET_SIZE) {
        return uproc_error(UPROC_ERRNO);
    }
    sz = uproc_io_write(&ecurve->ranks_count, sizeof ecurve->ranks_count, 1,
                        stream);
    if (sz != 1) {
        return uproc_error(UPROC_ERRNO);
    }
    sz = uproc_io_write(&ecurve->suffix_count, sizeof ecurve->suffix_count, 1,
                        stream);
    if (sz != 1) {
        return uproc_error(UPROC_ERRNO);
    }
    if (progress) {
        progress(0.1, progress_arg);
    }

    sz = uproc_io_write(ecurve->suffixes, sizeof *ecurve->suffixes,
                        ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        return uproc_error(UPROC_ERRNO);
    }
    if (progress) {
        progress(25.0, progress_arg);
    }
    sz = uproc_io_write(ecurve->classes, sizeof *ecurve->classes,
                        ecurve->ranks_count * ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        return uproc_error(UPROC_ERRNO);
    }
    if (progress) {
        progress(50.0, progress_arg);
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
            progress(50.0 + 50.0 / UPROC_PREFIX_MAX * i, progress_arg);
        }
    }
    if (progress) {
        progress(100.0, progress_arg);
    }
    return 0;
}
#endif

uproc_ecurve *uproc_ecurve_loads(enum uproc_ecurve_format format,
                                 uproc_io_stream *stream)
{
    return uproc_ecurve_loadps(format, NULL, NULL, stream);
}

uproc_ecurve *uproc_ecurve_loadps(enum uproc_ecurve_format format,
                                  void (*progress)(double, void *),
                                  void *progress_arg, uproc_io_stream *stream)
{
    if (format == UPROC_ECURVE_BINARY) {
#if HAVE_MMAP && USE_MMAP
        /* if mmap is enabled and, uproc_mmap_map is * called instead */
        uproc_error_msg(UPROC_EINVAL, "can't load mmap format from stream");
        return NULL;
#else
        return load_binary(stream, progress, progress_arg);
#endif
    }
    return load_plain(stream, progress, progress_arg);
}

uproc_ecurve *uproc_ecurve_loadv(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype, const char *pathfmt,
                                 va_list ap)
{
    return uproc_ecurve_loadpv(format, iotype, NULL, NULL, pathfmt, ap);
}

uproc_ecurve *uproc_ecurve_loadpv(enum uproc_ecurve_format format,
                                  enum uproc_io_type iotype,
                                  void (*progress)(double, void *),
                                  void *progress_arg, const char *pathfmt,
                                  va_list ap)
{
    uproc_assert(format == UPROC_ECURVE_PLAIN ||
                 format == UPROC_ECURVE_BINARY ||
                 format == UPROC_ECURVE_BINARY_V1);

    struct uproc_ecurve_s *ec;
    va_list aq;
    uproc_io_stream *stream;
    const char *mode[] = {
            [UPROC_ECURVE_BINARY] = "rb", [UPROC_ECURVE_BINARY_V1] = "rb",
            [UPROC_ECURVE_PLAIN] = "r",
    };

    va_copy(aq, ap);

#if HAVE_MMAP && USE_MMAP
    if (format != UPROC_ECURVE_PLAIN) {
        uproc_ecurve *ec = NULL;
        if (format == UPROC_ECURVE_BINARY_V1) {
            ec = uproc_ecurve_mmapv_v1(pathfmt, aq);
        } else {
            ec = uproc_ecurve_mmapv(pathfmt, aq);
        }
        if (ec && progress) {
            progress(100.0, progress_arg);
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
    ec = uproc_ecurve_loadps(format, progress, progress_arg, stream);
    uproc_io_close(stream);
    return ec;
}

uproc_ecurve *uproc_ecurve_load(enum uproc_ecurve_format format,
                                enum uproc_io_type iotype, const char *pathfmt,
                                ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_loadv(format, iotype, pathfmt, ap);
    va_end(ap);
    return ec;
}

uproc_ecurve *uproc_ecurve_loadp(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 void (*progress)(double, void *),
                                 void *progress_arg, const char *pathfmt, ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_loadpv(format, iotype, progress, progress_arg, pathfmt,
                             ap);
    va_end(ap);
    return ec;
}

int uproc_ecurve_stores(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        uproc_io_stream *stream)
{
    return uproc_ecurve_storeps(ecurve, format, NULL, NULL, stream);
}

int uproc_ecurve_storeps(const uproc_ecurve *ecurve,
                         enum uproc_ecurve_format format,
                         void (*progress)(double, void *), void *progress_arg,
                         uproc_io_stream *stream)
{
    if (format == UPROC_ECURVE_BINARY) {
#if HAVE_MMAP && USE_MMAP
        return uproc_error_msg(UPROC_EINVAL,
                               "can't store mmap format to stream");
#else
        return store_binary(ecurve, stream, progress, progress_arg);
#endif
    }
    return store_plain(ecurve, stream, progress, progress_arg);
}

int uproc_ecurve_storev(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap)
{
    return uproc_ecurve_storepv(ecurve, format, iotype, NULL, NULL, pathfmt,
                                ap);
}

int uproc_ecurve_storepv(const uproc_ecurve *ecurve,
                         enum uproc_ecurve_format format,
                         enum uproc_io_type iotype,
                         void (*progress)(double, void *), void *progress_arg,
                         const char *pathfmt, va_list ap)
{
    uproc_assert(format == UPROC_ECURVE_PLAIN || format == UPROC_ECURVE_BINARY);

    int res;
    va_list aq;
    uproc_io_stream *stream;
    static const char *mode[] = {
            [UPROC_ECURVE_BINARY] = "wb", [UPROC_ECURVE_PLAIN] = "w",
    };

    va_copy(aq, ap);

#if HAVE_MMAP && USE_MMAP
    if (format == UPROC_ECURVE_BINARY) {
        res = uproc_ecurve_mmap_storev(ecurve, pathfmt, aq);
        va_end(aq);
        if (!res && progress) {
            progress(100.0, progress_arg);
        }
        return res;
    }
#endif

    stream = uproc_io_openv(mode[format], iotype, pathfmt, aq);
    if (!stream) {
        return -1;
    }
    res = uproc_ecurve_storeps(ecurve, format, progress, progress_arg, stream);
    uproc_io_close(stream);
    return res;
}

int uproc_ecurve_store(const uproc_ecurve *ecurve,
                       enum uproc_ecurve_format format,
                       enum uproc_io_type iotype, const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_storev(ecurve, format, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

int uproc_ecurve_storep(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype,
                        void (*progress)(double, void *), void *progress_arg,
                        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_storepv(ecurve, format, iotype, progress, progress_arg,
                               pathfmt, ap);
    va_end(ap);
    return res;
}
