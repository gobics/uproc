#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "ecurve/common.h"
#include "ecurve/ecurve.h"
#include "ecurve/mmap.h"
#include "ecurve/word.h"
#include "ecurve/storage.h"
#include "pack.h"

#include "ecurve/gz.h"


/** Format of binary file header.
 * uint32_t: number of prefixes that have suffixes
 * uint64_t: total suffix count
 */
#define BIN_HEADER_FMT "L"

/** Binary format of a prefix.
 * uint32_t: prefix
 * uint64_t: number of suffixes associated with the prefix
 */
#define BIN_PREFIX_FMT "IL"

/** Binary format of a suffix.
 * uint64: suffix
 * uint16: associated class
 */
#define BIN_SUFFIX_FMT "LH"

/* buffer size must be large enough to handle all formats above */
#define BIN_BUFSZ 12

static int bin_load_header(gzFile stream, char *alpha, size_t *suffix_count);
static int bin_load_prefix(gzFile stream, const struct ec_alphabet *alpha,
                           ec_prefix *prefix, size_t *suffix_count);
static int bin_load_suffix(gzFile stream, const struct ec_alphabet *alpha,
                           ec_suffix *suffix, ec_class *cls);

static int bin_store_header(gzFile stream, const char *alpha, size_t suffix_count);
static int bin_store_prefix(gzFile stream, const struct ec_alphabet *alpha,
                            ec_prefix prefix, size_t suffix_count);
static int bin_store_suffix(gzFile stream, const struct ec_alphabet *alpha,
                            ec_suffix suffix, ec_class cls);

static int load_stream(struct ec_ecurve *ecurve, gzFile stream, int format);
static int store_stream(const struct ec_ecurve *ecurve, gzFile stream,
                        int format);

#define STR1(x) #x
#define STR(x) STR1(x)
#define PLAIN_BUFSZ 1024
#define PLAIN_COMMENT_CHAR '#'
#define PLAIN_HEADER_PRI ">> alphabet: %." STR(EC_ALPHABET_SIZE) "s, suffixes: %zu\n"
#define PLAIN_HEADER_SCN ">> alphabet: %"  STR(EC_ALPHABET_SIZE) "c, suffixes: %zu"

#define PLAIN_PREFIX_PRI ">%." STR(EC_PREFIX_LEN) "s %zu\n"
#define PLAIN_PREFIX_SCN ">%"  STR(EC_PREFIX_LEN) "c %zu"

#define PLAIN_SUFFIX_PRI "%." STR(EC_SUFFIX_LEN) "s %" EC_CLASS_PRI "\n"
#define PLAIN_SUFFIX_SCN "%"  STR(EC_SUFFIX_LEN) "c %" EC_CLASS_SCN

static int plain_read_line(gzFile stream, char *line, size_t n);

static int plain_load_header(gzFile stream, char *alpha, size_t *suffix_count);

static int plain_load_prefix(gzFile stream, const struct ec_alphabet *alpha,
                             ec_prefix *prefix, size_t *suffix_count);

static int plain_load_suffix(gzFile stream, const struct ec_alphabet *alpha,
                             ec_suffix *suffix, ec_class *cls);

static int plain_store_header(gzFile stream, const char *alpha,
                              size_t suffix_count);

static int plain_store_prefix(gzFile stream, const struct ec_alphabet *alpha,
                              ec_prefix prefix, size_t suffix_count);

static int plain_store_suffix(gzFile stream, const struct ec_alphabet *alpha,
                              ec_suffix suffix, ec_class cls);


static int
bin_load_header(gzFile stream, char *alpha, size_t *suffix_count)
{
    unsigned char buf[BIN_BUFSZ];
    int n = pack_bytes(BIN_HEADER_FMT);
    uint64_t tmp;

    if (gzread(stream, alpha, EC_ALPHABET_SIZE) != EC_ALPHABET_SIZE) {
        return EC_FAILURE;
    }
    alpha[EC_ALPHABET_SIZE] = '\0';

    if (gzread(stream, buf, n) != n) {
        return EC_FAILURE;
    }
    unpack(buf, BIN_HEADER_FMT, &tmp);
    *suffix_count = tmp;
    return EC_SUCCESS;
}

#define BIN_LOAD(NAME, FMT, T1, BIN1, T2, BIN2)                                 \
static int bin_load_ ## NAME(gzFile stream, const struct ec_alphabet *alpha,    \
                             T1 *out1, T2 *out2)                                \
{                                                                               \
    unsigned char buf[BIN_BUFSZ];                                               \
    BIN1 tmp1;                                                                  \
    BIN2 tmp2;                                                                  \
    int n = pack_bytes(FMT);                                                    \
    (void) alpha;                                                               \
    if (gzread(stream, buf, n) != n) {                                          \
        return EC_FAILURE;                                                      \
    }                                                                           \
    unpack(buf, FMT, &tmp1, &tmp2);                                             \
    *out1 = tmp1;                                                               \
    *out2 = tmp2;                                                               \
    return EC_SUCCESS;                                                          \
}


static int bin_store_header(gzFile stream, const char *alpha, size_t suffix_count)
{
    unsigned char buf[BIN_BUFSZ];
    int n;

    if (gzwrite(stream, alpha, EC_ALPHABET_SIZE) != EC_ALPHABET_SIZE) {
        return EC_FAILURE;
    }
    n = pack(buf, BIN_HEADER_FMT, (uint64_t)suffix_count);
    if (gzwrite(stream, buf, n) != n) {
        return EC_FAILURE;
    }
    return EC_SUCCESS;
}

#define BIN_STORE(NAME, FMT, T1, BIN1, T2, BIN2)                                \
static int bin_store_ ## NAME (gzFile stream, const struct ec_alphabet *alpha,  \
                               T1 in1, T2 in2)                                  \
{                                                                               \
    unsigned char buf[BIN_BUFSZ];                                               \
    int n = pack(buf, FMT, (BIN1)in1, (BIN2)in2);                               \
    (void) alpha;                                                               \
    return gzwrite(stream, buf, n) == n ? EC_SUCCESS : EC_FAILURE;              \
}
#define BIN(...) BIN_LOAD(__VA_ARGS__) BIN_STORE(__VA_ARGS__)
BIN(prefix, BIN_PREFIX_FMT, ec_prefix, uint32_t, size_t, uint64_t)
BIN(suffix, BIN_SUFFIX_FMT, ec_suffix, uint64_t, ec_class, uint16_t)


static int
plain_read_line(gzFile stream, char *line, size_t n)
{
    do {
        if (!gzgets(stream, line, n)) {
            return EC_FAILURE;
        }
    } while (line[0] == PLAIN_COMMENT_CHAR);
    return EC_SUCCESS;
}

static int
plain_load_header(gzFile stream, char *alpha, size_t *suffix_count)
{
    char line[PLAIN_BUFSZ];
    int res = plain_read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, PLAIN_HEADER_SCN, alpha, suffix_count);
    alpha[EC_ALPHABET_SIZE] = '\0';
    return res == 2 ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_load_prefix(gzFile stream, const struct ec_alphabet *alpha,
                  ec_prefix *prefix, size_t *suffix_count)
{
    int res;
    char line[PLAIN_BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;

    res = plain_read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = ec_word_to_string(word_str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, PLAIN_PREFIX_SCN, word_str, suffix_count);
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
plain_load_suffix(gzFile stream, const struct ec_alphabet *alpha,
                  ec_suffix *suffix, ec_class *cls)
{
    int res;
    char line[PLAIN_BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = EC_WORD_INITIALIZER;

    res = plain_read_line(stream, line, sizeof line);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = ec_word_to_string(word_str, &word, alpha);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, PLAIN_SUFFIX_SCN, &word_str[EC_PREFIX_LEN], cls);
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

static int
plain_store_header(gzFile stream, const char *alpha, size_t suffix_count)
{
    int res;
    res = gzprintf(stream, PLAIN_HEADER_PRI, alpha, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_store_prefix(gzFile stream, const struct ec_alphabet *alpha,
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
    res = gzprintf(stream, PLAIN_PREFIX_PRI, str, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_store_suffix(gzFile stream, const struct ec_alphabet *alpha,
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
    res = gzprintf(stream, PLAIN_SUFFIX_PRI, &str[EC_PREFIX_LEN], cls);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
load_stream(struct ec_ecurve *ecurve, gzFile stream, int format)
{
    int res;
    ec_prefix p;
    ec_suffix s;
    size_t suffix_count, prev_last;
    char alpha[EC_ALPHABET_SIZE + 1];
    struct load_funcs
    {
        int (*header)(gzFile, char *, size_t *);
        int (*prefix)(gzFile, const struct ec_alphabet *, ec_prefix *, size_t *);
        int (*suffix)(gzFile, const struct ec_alphabet *, ec_suffix *, ec_class *);
    } f;

    switch (format) {
        case EC_STORAGE_BINARY:
            f.header = &bin_load_header;
            f.prefix = &bin_load_prefix;
            f.suffix = &bin_load_suffix;
            break;
        case EC_STORAGE_PLAIN:
            f.header = &plain_load_header;
            f.prefix = &plain_load_prefix;
            f.suffix = &plain_load_suffix;
            break;
        default:
            return EC_FAILURE;
    }

    res = f.header(stream, alpha, &suffix_count);
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

        res = f.prefix(stream, &ecurve->alphabet, &prefix, &p_suffixes);
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
            res = f.suffix(stream, &ecurve->alphabet,
                           &ecurve->suffixes[s],
                           &ecurve->classes[s]);
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

int
ec_storage_load(struct ec_ecurve *ecurve, const char *path, int format)
{
    int res;
    gzFile stream;
    const char *mode[] = {
        [EC_STORAGE_BINARY] = "rb",
        [EC_STORAGE_PLAIN] = "r",
    };

    if (format == EC_STORAGE_MMAP) {
        return ec_mmap_map(ecurve, path);
    }

    stream = gzopen(path, mode[format]);
    if (!stream) {
        return EC_FAILURE;
    }
    (void) gzbuffer(stream, EC_GZ_BUFSZ);
    res = load_stream(ecurve, stream, format);
    gzclose(stream);
    return res;
}

static int
store_stream(const struct ec_ecurve *ecurve, gzFile stream, int format)
{
    int res;
    size_t p;
    struct store_funcs
    {
        int (*header)(gzFile, const char *, size_t);
        int (*prefix)(gzFile, const struct ec_alphabet *, ec_prefix, size_t);
        int (*suffix)(gzFile, const struct ec_alphabet *, ec_suffix, ec_class);
    } f;

    switch (format) {
        case EC_STORAGE_BINARY:
            f.header = &bin_store_header;
            f.prefix = &bin_store_prefix;
            f.suffix = &bin_store_suffix;
            break;
        case EC_STORAGE_PLAIN:
            f.header = &plain_store_header;
            f.prefix = &plain_store_prefix;
            f.suffix = &plain_store_suffix;
            break;
        default:
            return EC_FAILURE;
    }

    res = f.header(stream, ecurve->alphabet.str, ecurve->suffix_count);

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

        res = f.prefix(stream, &ecurve->alphabet, p, suffix_count);
        if (res != EC_SUCCESS) {
            return res;
        }

        for (i = 0; i < suffix_count; i++) {
            res = f.suffix(stream, &ecurve->alphabet,
                           ecurve->suffixes[offset + i],
                           ecurve->classes[offset + i]);
            if (res != EC_SUCCESS) {
                return res;
            }
        }
    }
    return EC_SUCCESS;
}

int
ec_storage_store(const struct ec_ecurve *ecurve, const char *path, int format,
        int flags)
{
    int res;
    gzFile stream;
    static const char *modes_avail[] = {
        [EC_STORAGE_BINARY] = "wb",
        [EC_STORAGE_PLAIN] = "w",
    };
    char mode[4] = "";

    if (format == EC_STORAGE_MMAP) {
        return ec_mmap_store(ecurve, path);
    }

    strcpy(mode, modes_avail[format]);
#if HAVE_ZLIB
    if (!(flags & EC_STORAGE_GZIP)) {
        strcat(mode, "T");
    }
#else
    (void) flags;
#endif

    stream = gzopen(path, mode);
    if (!stream) {
        return EC_FAILURE;
    }
    (void) gzbuffer(stream, EC_GZ_BUFSZ);
    res = store_stream(ecurve, stream, format);
    gzclose(stream);
    return res;
}
