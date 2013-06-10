#ifndef EC_COMMON_H
#define EC_COMMON_H

/** \file ecurve/common.h
 *
 * Common definitions
 */

#include <stdint.h>
#include <inttypes.h>

/** Return codes */
enum {
    /** Success! */
    EC_SUCCESS = 0,

    /** General failure */
    EC_FAILURE,

    /** Iterator exhausted */
    EC_ITER_STOP,
};

/** Length of the suffix part of a word */
#define EC_PREFIX_LEN 6

/** Length of the prefix part of a word */
#define EC_SUFFIX_LEN 12

/** Total word length */
#define EC_WORD_LEN (EC_PREFIX_LEN + EC_SUFFIX_LEN)


/** Type to represent one amino acid. */
typedef int ec_amino;

/** Bits needed to represent one amino acid */
#define EC_AMINO_BITS 5

/** Number of amino acids in the alphabet */
#define EC_ALPHABET_SIZE 20


/*----------*
 * PREFIXES *
 *----------*/

/** Type for prefixes
 *
 * Prefixes are (in contrast to suffixes, see below) contiguous, i.e. all values
 * from 0 to #EC_PREFIX_MAX represent valid prefixes.
 */
typedef uint32_t ec_prefix;
/** printf() format for prefixes */
#define EC_PREFIX_PRI PRIu32
/** scanf() format for prefixes */
#define EC_PREFIX_SCN SCNu32

/** Raise `x` to the power of 6 */
#define POW6(x) ((x) * (x) * (x) * (x) * (x) * (x))

/** Maximum value of a prefix */
#define EC_PREFIX_MAX (POW6((unsigned long) EC_ALPHABET_SIZE) - 1)


/*----------*
 * SUFFIXES *
 *----------*/

/** Type for suffixes
 *
 * Suffixes are represented as a "bit string" of #EC_SUFFIX_LEN amino acids,
 * each represented #EC_AMINO_BITS bits.
 */
typedef uint64_t ec_suffix;

/** printf() format for suffixes */
#define EC_SUFFIX_PRI PRIu64

/** scanf() format for suffixes */
#define EC_SUFFIX_SCN SCNu64

/** Select only the bits that are relevant for a suffix */
#define EC_SUFFIX_MASK(x) ((x) & ~(~0ULL << (EC_AMINO_BITS * EC_SUFFIX_LEN)))


/** Identifier of a protein class (or "protein family"). */
typedef uint16_t ec_class;

/** printf() format for #ec_class */
#define EC_CLASS_PRI PRIu16

/** scanf() format for #ec_class */
#define EC_CLASS_SCN SCNu16

#endif
