#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "ecurve/common.h"
#include "ecurve/ecurve.h"
#include "ecurve/word.h"
#include "pack.h"
#include <assert.h>

struct load_funcs
{
    int (*header)(FILE *, void *, ec_prefix *, ec_suffix *);
    int (*prefix)(FILE *, void *, ec_prefix *, ec_suffix *);
    int (*suffix)(FILE *, void *, ec_suffix *, ec_class *);
};

/** Load ecurve in sequential format. */
static int load(FILE *stream, ec_ecurve *ecurve, void *arg, struct load_funcs f);

struct store_funcs
{
     int (*header)(FILE *, void *, ec_prefix, ec_suffix);
     int (*prefix)(FILE *, void *, ec_prefix, ec_suffix);
     int (*suffix)(FILE *, void *, ec_suffix, ec_class);
};

/** Store ecurve in sequential format. */
static int store(FILE *stream, const ec_ecurve *ecurve, void *arg, struct store_funcs f);


/** Format of binary file header.
 * uint32_t: number of prefixes that have suffixes
 * uint64_t: total suffix count
 */
#define BIN_HEADER_FMT "IL"

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

static int bin_load_header(FILE *stream, void *arg, ec_prefix *prefix_count, ec_suffix *suffix_count);
static int bin_load_prefix(FILE *stream, void *arg, ec_prefix *prefix, ec_suffix *suffix_count);
static int bin_load_suffix(FILE *stream, void *arg, ec_suffix *suffix, ec_class *cls);

static int bin_store_header(FILE *stream, void *arg, ec_prefix prefix_count, ec_suffix suffix_count);
static int bin_store_prefix(FILE *stream, void *arg, ec_prefix prefix, ec_suffix suffix_count);
static int bin_store_suffix(FILE *stream, void *arg, ec_suffix suffix, ec_class cls);


#define STR1(x) #x
#define STR(x) STR1(x)
#define PLAIN_BUFSZ 1024
#define PLAIN_HEADER_PRI ">> %" EC_PREFIX_PRI " %" EC_SUFFIX_PRI "\n"
#define PLAIN_HEADER_SCN ">> %" EC_PREFIX_SCN " %" EC_SUFFIX_SCN

#define PLAIN_PREFIX_PRI "> %." STR(EC_PREFIX_LEN) "s: %" EC_SUFFIX_PRI "\n"
#define PLAIN_PREFIX_SCN "> %" STR(EC_PREFIX_LEN) "c: %" EC_SUFFIX_SCN

#define PLAIN_SUFFIX_PRI "%." STR(EC_SUFFIX_LEN) "s: %" EC_CLASS_PRI "\n"
#define PLAIN_SUFFIX_SCN "%" STR(EC_SUFFIX_LEN) "c: %" EC_CLASS_SCN

static int plain_read_line(FILE *stream, char *line, size_t n);
static int plain_load_header(FILE *stream, void *arg, ec_prefix *prefix_count, ec_suffix *suffix_count);
static int plain_load_prefix(FILE *stream, void *arg, ec_prefix *prefix, ec_suffix *suffix_count);
static int plain_load_suffix(FILE *stream, void *arg, ec_suffix *suffix, ec_class *cls);

static int plain_store_header(FILE *stream, void *arg, ec_prefix prefix_count, ec_suffix suffix_count);
static int plain_store_prefix(FILE *stream, void *arg, ec_prefix prefix, ec_suffix suffix_count);
static int plain_store_suffix(FILE *stream, void *arg, ec_suffix suffix, ec_class cls);


static int
load(FILE *stream, ec_ecurve *ecurve, void *arg, struct load_funcs f)
{
    int res;
    ec_prefix p, prefix_count;
    ec_suffix s, suffix_count, *prev_last;

    (void) arg;

    res = f.header(stream, arg, &prefix_count, &suffix_count);
    if (res != EC_SUCCESS) {
        return res;
    }

    res = ec_ecurve_init(ecurve, suffix_count);
    if (res != EC_SUCCESS) {
        return res;
    }

    for (prev_last = NULL, s = p = 0; s < suffix_count;) {
        ec_prefix prefix;
        ec_suffix ps, p_suffixes;

        res = f.prefix(stream, arg, &prefix, &p_suffixes);
        if (res != EC_SUCCESS) {
            return res;
        }

        for (; p < prefix; p++) {
            ecurve->prefix_table[p].first = prev_last;
            ecurve->prefix_table[p].count = 0;
        }
        ecurve->prefix_table[prefix].first = &ecurve->suffix_table[s];
        ecurve->prefix_table[prefix].count = p_suffixes;

        for (ps = 0; ps < p_suffixes; ps++, s++) {
            res = f.suffix(stream, arg,
                           &ecurve->suffix_table[s],
                           &ecurve->class_table[s]);
            if (res != EC_SUCCESS) {
                return res;
            }
        }
        prev_last = &ecurve->suffix_table[s - 1];
    }
    for (; p < EC_PREFIX_MAX; p++) {
        ecurve->prefix_table[p].first = prev_last;
        ecurve->prefix_table[p].count = -1;
    }


    return EC_SUCCESS;
}

static int
store(FILE *stream, const ec_ecurve *ecurve, void *arg, struct store_funcs f)
{
    int res;
    size_t p, prefix_count;
    (void) arg;

    for (prefix_count = p = 0; p <= EC_PREFIX_MAX; p++) {
        if (ecurve->prefix_table[p].count != 0 &&
            ecurve->prefix_table[p].count != (size_t) -1
           ) {
            prefix_count++;
        }
    }
    assert(prefix_count > 0);

    res = f.header(stream, arg, prefix_count, ecurve->suffix_count);

    if (res != EC_SUCCESS) {
        return res;
    }

    for (p = 0; p <= EC_PREFIX_MAX; p++) {
        ec_suffix *first = ecurve->prefix_table[p].first;
        size_t suffix_count = ecurve->prefix_table[p].count;
        size_t offset = first - ecurve->suffix_table;
        size_t i; 

        if (!suffix_count || suffix_count == (size_t) -1) {
            continue;
        }

        res = f.prefix(stream, arg, p, suffix_count);
        if (res != EC_SUCCESS) {
            return res;
        }

        for (i = 0; i < suffix_count; i++) {
            res = f.suffix(stream, arg,
                           ecurve->suffix_table[offset + i],
                           ecurve->class_table[offset + i]);
            if (res != EC_SUCCESS) {
                return res;
            }
        }
    }
    return EC_SUCCESS;
}

#define BIN_LOAD(NAME, FMT, T1, BIN1, T2, BIN2)                             \
static int bin_load_ ## NAME(FILE *stream, void *p, T1 *out1, T2 *out2)     \
{                                                                           \
    unsigned char buf[BIN_BUFSZ];                                           \
    BIN1 tmp1;                                                              \
    BIN2 tmp2;                                                              \
    size_t n = pack_bytes(FMT);                                             \
    (void) p;                                                               \
    if (fread(buf, 1, n, stream) != n) {                                    \
        return EC_FAILURE;                                                  \
    }                                                                       \
    unpack(buf, FMT, &tmp1, &tmp2);                                         \
    *out1 = tmp1;                                                           \
    *out2 = tmp2;                                                           \
    return EC_SUCCESS;                                                      \
}

#define BIN_STORE(NAME, FMT, T1, BIN1, T2, BIN2)                            \
static int bin_store_ ## NAME (FILE *stream, void *p, T1 in1, T2 in2)       \
{                                                                           \
    unsigned char buf[BIN_BUFSZ];                                           \
    size_t n = pack(buf, FMT, (BIN1)in1, (BIN2)in2);                        \
    (void) p;                                                               \
    return fwrite(buf, 1, n, stream) == n ? EC_SUCCESS : EC_FAILURE;        \
}
#define BIN(...) BIN_LOAD(__VA_ARGS__) BIN_STORE(__VA_ARGS__)
BIN(header, BIN_HEADER_FMT, ec_prefix, uint32_t, ec_suffix, uint64_t)
BIN(prefix, BIN_PREFIX_FMT, ec_prefix, uint32_t, ec_suffix, uint64_t)
BIN(suffix, BIN_SUFFIX_FMT, ec_suffix, uint64_t, ec_class, uint16_t)


static int
plain_read_line(FILE *stream, char *line, size_t n)
{
    do {
        if (!fgets(line, n, stream)) {
            return EC_FAILURE;
        }
    } while (line[0] == '#');
    return EC_SUCCESS;
}

static int
plain_load_header(FILE *stream, void *arg, ec_prefix *prefix_count,
                  ec_suffix *suffix_count)
{
    char line[PLAIN_BUFSZ];
    int res = plain_read_line(stream, line, sizeof line);
    (void) arg;
    if (res != EC_SUCCESS) {
        return res;
    }
    res = sscanf(line, PLAIN_HEADER_SCN, prefix_count, suffix_count);
    return (res == 2) ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_load_prefix(FILE *stream, void *arg, ec_prefix *prefix,
                  ec_suffix *suffix_count)
{
    int res;
    char line[PLAIN_BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = { 0, 0 };
    ec_alphabet *alpha = arg;
        
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
plain_load_suffix(FILE *stream, void *arg, ec_suffix *suffix,
                  ec_class *cls)
{
    int res;
    char line[PLAIN_BUFSZ], word_str[EC_WORD_LEN + 1];
    struct ec_word word = { 0, 0 };
    ec_alphabet *alpha = arg;
        
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
plain_store_header(FILE *stream, void *arg, ec_prefix prefix_count,
                  ec_suffix suffix_count)
{
    int res;
    (void) arg;
    res = fprintf(stream, PLAIN_HEADER_PRI, prefix_count, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_store_prefix(FILE *stream, void *arg, ec_prefix prefix,
                  ec_suffix suffix_count)
{
    int res;
    char str[EC_WORD_LEN + 1];
    struct ec_word word = { prefix, 0 };
    ec_alphabet *alpha = arg;

    res = ec_word_to_string(str, &word, alpha);
    str[EC_PREFIX_LEN] = '\0';
    res = fprintf(stream, PLAIN_PREFIX_PRI, str, suffix_count);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}

static int
plain_store_suffix(FILE *stream, void *arg, ec_suffix suffix,
                  ec_class cls)
{
    int res;
    char str[EC_WORD_LEN + 1];
    struct ec_word word = { 0, suffix };
    ec_alphabet *alpha = arg;
    res = ec_word_to_string(str, &word, alpha);
    res = fprintf(stream, PLAIN_SUFFIX_PRI, &str[EC_PREFIX_LEN], cls);
    return (res > 0) ? EC_SUCCESS : EC_FAILURE;
}


/*------*
 * load *
 *------*/

int
ec_ecurve_load(ec_ecurve *ecurve, const char *path,
               int (*load)(ec_ecurve *, FILE*, void *), void *arg)
{
    int res;
    FILE *stream;

    stream = fopen(path, "r");
    if (!stream) {
        return EC_FAILURE;
    }

    res = (*load)(ecurve, stream, arg);
    fclose(stream);

    return res;
}

int
ec_ecurve_load_binary(ec_ecurve *ecurve, FILE *stream, void *arg)
{
    struct load_funcs f = { &bin_load_header, &bin_load_prefix, &bin_load_suffix };
    return load(stream, ecurve, arg, f);
}

int
ec_ecurve_load_plain(ec_ecurve *ecurve, FILE *stream, void *arg)
{
    struct load_funcs f = { &plain_load_header, &plain_load_prefix, &plain_load_suffix };
    return load(stream, ecurve, arg, f);
}


/*-------*
 * store *
 *-------*/

int
ec_ecurve_store(const ec_ecurve *ecurve, const char *path,
                int (*store)(const ec_ecurve *, FILE *, void *), void *arg)
{
    int res;
    FILE *stream;

    stream = fopen(path, "w");
    if (!stream) {
        return EC_FAILURE;
    }

    res = (*store)(ecurve, stream, arg);
    fclose(stream);

    return res;
}

int
ec_ecurve_store_binary(const ec_ecurve *ecurve, FILE *stream, void *arg)
{
    struct store_funcs f = { &bin_store_header, &bin_store_prefix, &bin_store_suffix };
    return store(stream, ecurve, arg, f);
}

int
ec_ecurve_store_plain(const ec_ecurve *ecurve, FILE *stream, void *arg)
{
    struct store_funcs f = { &plain_store_header, &plain_store_prefix, &plain_store_suffix };
    return store(stream, ecurve, arg, f);
}
