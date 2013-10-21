#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "ecurve/common.h"
#include "ecurve/ecurve.h"
#include "ecurve/io.h"
#include "ecurve/mmap.h"
#include "ecurve/word.h"
#include "ecurve/storage.h"


#define STR1(x) #x
#define STR(x) STR1(x)
#define BUFSZ 1024
#define COMMENT_CHAR '#'
#define HEADER_PRI ">> alphabet: %." STR(EC_ALPHABET_SIZE) "s, suffixes: %zu\n"
#define HEADER_SCN ">> alphabet: %"  STR(EC_ALPHABET_SIZE) "c, suffixes: %zu"

#define PREFIX_PRI ">%." STR(EC_PREFIX_LEN) "s %zu\n"
#define PREFIX_SCN ">%"  STR(EC_PREFIX_LEN) "c %zu"

#define SUFFIX_PRI "%." STR(EC_SUFFIX_LEN) "s %" EC_CLASS_PRI "\n"
#define SUFFIX_SCN "%"  STR(EC_SUFFIX_LEN) "c %" EC_CLASS_SCN

static int
read_line(ec_io_stream *stream, char *line, size_t n)
{
    do {
        if (!ec_io_gets(line, n, stream)) {
            return EC_FAILURE;
        }
    } while (line[0] == COMMENT_CHAR);
    return EC_SUCCESS;
}

static int
load_header(ec_io_stream *stream, char *alpha, size_t *suffix_count)
{
    char line[BUFSZ];
    int res = read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, HEADER_SCN, alpha, suffix_count);
    alpha[EC_ALPHABET_SIZE] = '\0';
    return res == 2 ? EC_SUCCESS : EC_FAILURE;
}

static int
load_prefix(ec_io_stream *stream, const struct ec_alphabet *alpha,
                  ec_prefix *prefix, size_t *suffix_count)
{
    int res;
    char line[BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;

    res = read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = ec_word_to_string(word_str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, PREFIX_SCN, word_str, suffix_count);
    if (res != 2) {
        return EC_FAILURE;
    }
    res = ec_word_from_string(&word, word_str, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    *prefix = word.prefix;
    return EC_SUCCESS;
}

static int
load_suffix(ec_io_stream *stream, const struct ec_alphabet *alpha,
                  ec_suffix *suffix, ec_class *cls)
{
    int res;
    char line[BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;

    res = read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = ec_word_to_string(word_str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, SUFFIX_SCN, &word_str[EC_PREFIX_LEN], cls);
    if (res != 2) {
        return EC_FAILURE;
    }
    res = ec_word_from_string(&word, word_str, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    *suffix = word.suffix;
    return EC_SUCCESS;
}

int
ec_storage_plain_load(struct ec_ecurve *ecurve, ec_io_stream *stream)
{
    int res;
    ec_prefix p;
    ec_suffix s;
    size_t suffix_count, prev_last;
    char alpha[EC_ALPHABET_SIZE + 1];
    res = load_header(stream, alpha, &suffix_count);

    if (res != EC_SUCCESS) {
        return res;
    }

    res = ec_ecurve_init(ecurve, alpha, suffix_count);
    if (res != EC_SUCCESS) {
        return res;
    }

    for (prev_last = 0, s = p = 0; s < suffix_count;) {
        ec_prefix prefix;
        size_t ps, p_suffixes;

        res = load_prefix(stream, &ecurve->alphabet, &prefix, &p_suffixes);
        if (res != EC_SUCCESS) {
            return res;
        }

        if (prefix > EC_PREFIX_MAX) {
            return EC_EINVAL;
        }

        for (; p < prefix; p++) {
            ecurve->prefixes[p].first = prev_last;
            ecurve->prefixes[p].count = prev_last ? 0 : EC_ECURVE_EDGE;
        }
        ecurve->prefixes[prefix].first = s;
        ecurve->prefixes[prefix].count = p_suffixes;
        p++;

        for (ps = 0; ps < p_suffixes; ps++, s++) {
            res = load_suffix(stream, &ecurve->alphabet,
                    &ecurve->suffixes[s], &ecurve->classes[s]);
            if (res != EC_SUCCESS) {
                return res;
            }
        }
        prev_last = s - 1;
    }
    for (; p < EC_PREFIX_MAX + 1; p++) {
        ecurve->prefixes[p].first = prev_last;
        ecurve->prefixes[p].count = EC_ECURVE_EDGE;
    }
    return EC_SUCCESS;
}

static int
store_header(ec_io_stream *stream, const char *alpha, size_t suffix_count)
{
    int res;
    res = ec_io_printf(stream, HEADER_PRI, alpha, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
store_prefix(ec_io_stream *stream, const struct ec_alphabet *alpha,
        ec_prefix prefix, size_t suffix_count)
{
    int res;
    char str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;
    word.prefix = prefix;
    res = ec_word_to_string(str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    str[EC_PREFIX_LEN] = '\0';
    res = ec_io_printf(stream, PREFIX_PRI, str, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
store_suffix(ec_io_stream *stream, const struct ec_alphabet *alpha,
        ec_suffix suffix, ec_class cls)
{
    int res;
    char str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;
    word.suffix = suffix;
    res = ec_word_to_string(str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = ec_io_printf(stream, SUFFIX_PRI, &str[EC_PREFIX_LEN], cls);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

int
ec_storage_plain_store(const struct ec_ecurve *ecurve, ec_io_stream *stream)
{
    int res;
    size_t p;

    res = store_header(stream, ecurve->alphabet.str, ecurve->suffix_count);

    if (res != EC_SUCCESS) {
        return res;
    }

    for (p = 0; p <= EC_PREFIX_MAX; p++) {
        size_t suffix_count = ecurve->prefixes[p].count;
        size_t offset = ecurve->prefixes[p].first;
        size_t i;

        if (!suffix_count || EC_ECURVE_ISEDGE(ecurve->prefixes[p])) {
            continue;
        }

        res = store_prefix(stream, &ecurve->alphabet, p, suffix_count);
        if (res != EC_SUCCESS) {
            return res;
        }

        for (i = 0; i < suffix_count; i++) {
            res = store_suffix(stream, &ecurve->alphabet,
                    ecurve->suffixes[offset + i], ecurve->classes[offset + i]);
            if (res != EC_SUCCESS) {
                return res;
            }
        }
    }
    return EC_SUCCESS;
}


int
ec_storage_binary_load(struct ec_ecurve *ecurve, ec_io_stream *stream)
{
    int res;
    size_t sz;
    size_t suffix_count;
    char alpha[EC_ALPHABET_SIZE + 1];

    sz = ec_io_read(alpha, sizeof *alpha, EC_ALPHABET_SIZE, stream);
    if (sz != EC_ALPHABET_SIZE) {
        return EC_ESYSCALL;
    }
    alpha[EC_ALPHABET_SIZE] = '\0';

    sz = ec_io_read(&suffix_count, sizeof suffix_count, 1, stream);
    if (sz != 1) {
        return EC_ESYSCALL;
    }

    res = ec_ecurve_init(ecurve, alpha, suffix_count);
    if (EC_ISERROR(res)) {
        return res;
    }

    sz = ec_io_read(ecurve->suffixes, sizeof *ecurve->suffixes, suffix_count,
            stream);
    if (sz != suffix_count) {
        res = EC_ESYSCALL;
        goto error;
    }
    sz = ec_io_read(ecurve->classes, sizeof *ecurve->classes, suffix_count,
            stream);
    if (sz != suffix_count) {
        res = EC_ESYSCALL;
        goto error;
    }

    for (ec_prefix i = 0; i <= EC_PREFIX_MAX; i++) {
        sz = ec_io_read(&ecurve->prefixes[i].first,
                sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            res = EC_ESYSCALL;
            goto error;
        }
        sz = ec_io_read(&ecurve->prefixes[i].count,
                sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            res = EC_ESYSCALL;
            goto error;
        }
    }

    return EC_SUCCESS;
error:
    ec_ecurve_destroy(ecurve);
    return res;
}

int
ec_storage_binary_store(const struct ec_ecurve *ecurve, ec_io_stream *stream)
{
    int res;
    size_t sz;
    struct ec_alphabet alpha;

    ec_ecurve_get_alphabet(ecurve, &alpha);
    sz = ec_io_write(alpha.str, 1, EC_ALPHABET_SIZE, stream);
    if (sz != EC_ALPHABET_SIZE) {
        return EC_ESYSCALL;
    }

    sz = ec_io_write(&ecurve->suffix_count, sizeof ecurve->suffix_count, 1,
            stream);
    if (sz != 1) {
        return EC_ESYSCALL;
    }

    sz = ec_io_write(ecurve->suffixes, sizeof *ecurve->suffixes,
            ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        res = EC_ESYSCALL;
        goto error;
    }
    sz = ec_io_write(ecurve->classes, sizeof *ecurve->classes,
            ecurve->suffix_count, stream);
    if (sz != ecurve->suffix_count) {
        res = EC_ESYSCALL;
        goto error;
    }

    for (ec_prefix i = 0; i <= EC_PREFIX_MAX; i++) {
        sz = ec_io_write(&ecurve->prefixes[i].first,
                sizeof ecurve->prefixes[i].first, 1, stream);
        if (sz != 1) {
            res = EC_ESYSCALL;
            goto error;
        }
        sz = ec_io_write(&ecurve->prefixes[i].count,
                sizeof ecurve->prefixes[i].count, 1, stream);
        if (sz != 1) {
            res = EC_ESYSCALL;
            goto error;
        }
    }

    return EC_SUCCESS;
error:
    return res;
}


int
ec_storage_load(struct ec_ecurve *ecurve, const char *path,
        enum ec_storage_format format, enum ec_io_type iotype)
{
    int res;
    ec_io_stream *stream;
    const char *mode[] = {
        [EC_STORAGE_BINARY] = "rb",
        [EC_STORAGE_PLAIN] = "r",
    };

    if (format == EC_STORAGE_MMAP) {
        return ec_mmap_map(ecurve, path);
    }

    stream = ec_io_open(path, mode[format], iotype);
    if (!stream) {
        return EC_FAILURE;
    }
    if (format == EC_STORAGE_BINARY) {
        res = ec_storage_binary_load(ecurve, stream);
    }
    else {
        res = ec_storage_plain_load(ecurve, stream);
    }
    ec_io_close(stream);
    return res;
}

int
ec_storage_store(const struct ec_ecurve *ecurve, const char *path,
        enum ec_storage_format format, enum ec_io_type iotype)
{
    int res;
    ec_io_stream *stream;
    static const char *mode[] = {
        [EC_STORAGE_BINARY] = "wb",
        [EC_STORAGE_PLAIN] = "w",
    };

    if (format == EC_STORAGE_MMAP) {
        return ec_mmap_store(ecurve, path);
    }

    stream = ec_io_open(path, mode[format], iotype);
    if (!stream) {
        return EC_FAILURE;
    }
    if (format == EC_STORAGE_BINARY) {
        res = ec_storage_binary_store(ecurve, stream);
    }
    else {
        res = ec_storage_plain_store(ecurve, stream);
    }
    ec_io_close(stream);
    return res;
}
