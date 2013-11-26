/* Load/store ecurves from/to files
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
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"
#include "uproc/io.h"
#include "uproc/mmap.h"
#include "uproc/word.h"
#include "uproc/storage.h"


#define STR1(x) #x
#define STR(x) STR1(x)
#define BUFSZ 1024
#define COMMENT_CHAR '#'
#define HEADER_PRI ">> alphabet: %." STR(UPROC_ALPHABET_SIZE) "s, suffixes: %zu\n"
#define HEADER_SCN ">> alphabet: %"  STR(UPROC_ALPHABET_SIZE) "c, suffixes: %zu"

#define PREFIX_PRI ">%." STR(UPROC_PREFIX_LEN) "s %zu\n"
#define PREFIX_SCN ">%"  STR(UPROC_PREFIX_LEN) "c %zu"

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
load_header(uproc_io_stream *stream, char *alpha, size_t *suffix_count)
{
    char line[BUFSZ];
    int res = read_line(stream, line, sizeof line);
    if (res) {
        return res;
    }
    res = sscanf(line, HEADER_SCN, alpha, suffix_count);
    alpha[UPROC_ALPHABET_SIZE] = '\0';
    if (res != 2) {
        return uproc_error_msg(UPROC_EINVAL, "invalid header: \"%s\"", line);
    }
    return 0;
}

static int
load_prefix(uproc_io_stream *stream, const struct uproc_alphabet *alpha,
                  uproc_prefix *prefix, size_t *suffix_count)
{
    int res;
    char line[BUFSZ], word_str[UPROC_WORD_LEN + 1];
    struct uproc_word word = UPROC_WORD_INITIALIZER;

    res = read_line(stream, line, sizeof line);
    if (res) {
        return res;
    }
    res = uproc_word_to_string(word_str, &word, alpha);
    if (res) {
        return res;
    }
    res = sscanf(line, PREFIX_SCN, word_str, suffix_count);
    if (res != 2) {
        return uproc_error_msg(UPROC_EINVAL, "invalid prefix string: \"%s\"",
                              line);
    }
    res = uproc_word_from_string(&word, word_str, alpha);
    if (res) {
        return res;
    }
    *prefix = word.prefix;
    return 0;
}

static int
load_suffix(uproc_io_stream *stream, const struct uproc_alphabet *alpha,
                  uproc_suffix *suffix, uproc_family *family)
{
    int res;
    char line[BUFSZ], word_str[UPROC_WORD_LEN + 1];
    struct uproc_word word = UPROC_WORD_INITIALIZER;

    res = read_line(stream, line, sizeof line);
    if (res) {
        return res;
    }
    res = uproc_word_to_string(word_str, &word, alpha);
    if (res) {
        return res;
    }
    res = sscanf(line, SUFFIX_SCN, &word_str[UPROC_PREFIX_LEN], family);
    if (res != 2) {
        return uproc_error_msg(UPROC_EINVAL, "invalid suffix string: \"%s\"",
                              line);
    }
    res = uproc_word_from_string(&word, word_str, alpha);
    if (res) {
        return res;
    }
    *suffix = word.suffix;
    return 0;
}

static int
uproc_storage_plain_load(struct uproc_ecurve *ecurve, uproc_io_stream *stream)
{
    int res;
    uproc_prefix p;
    uproc_suffix s;
    size_t suffix_count, prev_last;
    char alpha[UPROC_ALPHABET_SIZE + 1];
    res = load_header(stream, alpha, &suffix_count);

    if (res) {
        return res;
    }

    res = uproc_ecurve_init(ecurve, alpha, suffix_count);
    if (res) {
        return res;
    }

    for (prev_last = 0, s = p = 0; s < suffix_count;) {
        uproc_prefix prefix = -1;
        size_t ps, p_suffixes;

        res = load_prefix(stream, &ecurve->alphabet, &prefix, &p_suffixes);
        if (res) {
            return res;
        }

        if (prefix > UPROC_PREFIX_MAX) {
            return uproc_error_msg(UPROC_EINVAL,
                                  "invalid prefix value: %" UPROC_PREFIX_PRI,
                                  prefix);
        }

        for (; p < prefix; p++) {
            ecurve->prefixes[p].first = prev_last;
            ecurve->prefixes[p].count = prev_last ? 0 : UPROC_ECURVE_EDGE;
        }
        ecurve->prefixes[prefix].first = s;
        ecurve->prefixes[prefix].count = p_suffixes;
        p++;

        for (ps = 0; ps < p_suffixes; ps++, s++) {
            res = load_suffix(stream, &ecurve->alphabet,
                    &ecurve->suffixes[s], &ecurve->families[s]);
            if (res) {
                return res;
            }
        }
        prev_last = s - 1;
    }
    for (; p < UPROC_PREFIX_MAX + 1; p++) {
        ecurve->prefixes[p].first = prev_last;
        ecurve->prefixes[p].count = UPROC_ECURVE_EDGE;
    }
    return 0;
}

static int
store_header(uproc_io_stream *stream, const char *alpha, size_t suffix_count)
{
    int res;
    res = uproc_io_printf(stream, HEADER_PRI, alpha, suffix_count);
    return (res > 0) ? 0 : -1;
}

static int
store_prefix(uproc_io_stream *stream, const struct uproc_alphabet *alpha,
        uproc_prefix prefix, size_t suffix_count)
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
    res = uproc_io_printf(stream, PREFIX_PRI, str, suffix_count);
    return (res > 0) ? 0 : -1;
}

static int
store_suffix(uproc_io_stream *stream, const struct uproc_alphabet *alpha,
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
uproc_storage_plain_store(const struct uproc_ecurve *ecurve, uproc_io_stream *stream)
{
    int res;
    size_t p;

    res = store_header(stream, ecurve->alphabet.str, ecurve->suffix_count);

    if (res) {
        return res;
    }

    for (p = 0; p <= UPROC_PREFIX_MAX; p++) {
        size_t suffix_count = ecurve->prefixes[p].count;
        size_t offset = ecurve->prefixes[p].first;
        size_t i;

        if (!suffix_count || UPROC_ECURVE_ISEDGE(ecurve->prefixes[p])) {
            continue;
        }

        res = store_prefix(stream, &ecurve->alphabet, p, suffix_count);
        if (res) {
            return res;
        }

        for (i = 0; i < suffix_count; i++) {
            res = store_suffix(stream, &ecurve->alphabet,
                    ecurve->suffixes[offset + i], ecurve->families[offset + i]);
            if (res) {
                return res;
            }
        }
    }
    return 0;
}


static int
uproc_storage_binary_load(struct uproc_ecurve *ecurve, uproc_io_stream *stream)
{
    int res;
    size_t sz;
    size_t suffix_count;
    char alpha[UPROC_ALPHABET_SIZE + 1];

    sz = uproc_io_read(alpha, sizeof *alpha, UPROC_ALPHABET_SIZE, stream);
    if (sz != UPROC_ALPHABET_SIZE) {
        return uproc_error(UPROC_ERRNO);
    }
    alpha[UPROC_ALPHABET_SIZE] = '\0';

    sz = uproc_io_read(&suffix_count, sizeof suffix_count, 1, stream);
    if (sz != 1) {
        return uproc_error(UPROC_ERRNO);
    }

    res = uproc_ecurve_init(ecurve, alpha, suffix_count);
    if (res) {
        return res;
    }

    sz = uproc_io_read(ecurve->suffixes, sizeof *ecurve->suffixes, suffix_count,
            stream);
    if (sz != suffix_count) {
        res = uproc_error(UPROC_ERRNO);
        goto error;
    }
    sz = uproc_io_read(ecurve->families, sizeof *ecurve->families, suffix_count,
            stream);
    if (sz != suffix_count) {
        res = uproc_error(UPROC_ERRNO);
        goto error;
    }

    for (uproc_prefix i = 0; i <= UPROC_PREFIX_MAX; i++) {
        sz = uproc_io_read(&ecurve->prefixes[i].first,
                sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            res = uproc_error(UPROC_ERRNO);
            goto error;
        }
        sz = uproc_io_read(&ecurve->prefixes[i].count,
                sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            res = uproc_error(UPROC_ERRNO);
            goto error;
        }
    }

    return 0;
error:
    uproc_ecurve_destroy(ecurve);
    return res;
}

static int
uproc_storage_binary_store(const struct uproc_ecurve *ecurve, uproc_io_stream *stream)
{
    int res;
    size_t sz;
    struct uproc_alphabet alpha;

    uproc_ecurve_get_alphabet(ecurve, &alpha);
    sz = uproc_io_write(alpha.str, 1, UPROC_ALPHABET_SIZE, stream);
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
        res = UPROC_ERRNO;
        goto error;
    }
    sz = uproc_io_write(ecurve->families, sizeof *ecurve->families,
            ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        res = uproc_error(UPROC_ERRNO);
        goto error;
    }

    for (uproc_prefix i = 0; i <= UPROC_PREFIX_MAX; i++) {
        sz = uproc_io_write(&ecurve->prefixes[i].first,
                sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            res = uproc_error(UPROC_ERRNO);
            goto error;
        }
        sz = uproc_io_write(&ecurve->prefixes[i].count,
                sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            res = uproc_error(UPROC_ERRNO);
            goto error;
        }
    }

    return 0;
error:
    return res;
}

int
uproc_storage_loadv(struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    va_list aq;
    uproc_io_stream *stream;
    const char *mode[] = {
        [UPROC_STORAGE_BINARY] = "rb",
        [UPROC_STORAGE_PLAIN] = "r",
    };

    va_copy(aq, ap);

#if HAVE_MMAP
    if (format == UPROC_STORAGE_BINARY) {
        res = uproc_mmap_mapv(ecurve, pathfmt, aq);
        va_end(aq);
        return res;
    }
#endif

    stream = uproc_io_openv(mode[format], iotype, pathfmt, aq);
    va_end(aq);
    if (!stream) {
        return -1;
    }
    if (format == UPROC_STORAGE_BINARY) {
        res = uproc_storage_binary_load(ecurve, stream);
    }
    else {
        res = uproc_storage_plain_load(ecurve, stream);
    }
    uproc_io_close(stream);
    return res;
}
int
uproc_storage_load(struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_storage_loadv(ecurve, format, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

int
uproc_storage_storev(const struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    va_list aq;
    uproc_io_stream *stream;
    static const char *mode[] = {
        [UPROC_STORAGE_BINARY] = "wb",
        [UPROC_STORAGE_PLAIN] = "w",
    };

    va_copy(aq, ap);

#if HAVE_MMAP
    if (format == UPROC_STORAGE_BINARY) {
        res = uproc_mmap_storev(ecurve, pathfmt, aq);
        va_end(aq);
        return res;
    }
#endif

    stream = uproc_io_openv(mode[format], iotype, pathfmt, aq);
    if (!stream) {
        return -1;
    }
    if (format == UPROC_STORAGE_BINARY) {
        res = uproc_storage_binary_store(ecurve, stream);
    }
    else {
        res = uproc_storage_plain_store(ecurve, stream);
    }
    uproc_io_close(stream);
    return res;
}

int
uproc_storage_store(const struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_storage_storev(ecurve, format, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}
