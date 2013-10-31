#ifndef UPRO_COMMON_H
#define UPRO_COMMON_H

/** \file upro/common.h
 *
 * Common definitions
 */

#include <stdint.h>
#include <inttypes.h>
#include <math.h>

/** Return codes */
enum {
    /** Success! */
    UPRO_SUCCESS = 0,

    /** General failure */
    UPRO_FAILURE,

    /** Iterator produced an item */
    UPRO_ITER_YIELD,

    /** Iterator exhausted */
    UPRO_ITER_STOP,

    /** Memory allocation failed */
    UPRO_ENOMEM,

    /** Invalid argument */
    UPRO_EINVAL,

    /** Object doesn't exist  */
    UPRO_ENOENT,

    /** Object already exists */
    UPRO_EEXIST,

    /** A system call (that sets `errno`) returned an error */
    UPRO_ESYSCALL,
};

/** Test return code for error */
#define UPRO_ISERROR(e) ((e) != UPRO_SUCCESS && (e) != UPRO_ITER_YIELD && (e) != UPRO_ITER_STOP)

/** Epsilon value for comparing floating point numbers */
#define UPRO_EPSILON 1e-5

/** Lowest `n` bits set to one */
#define UPRO_BITMASK(n) (~(~0ULL << (n)))

/** Length of the suffix part of a word */
#define UPRO_PREFIX_LEN 6

/** Length of the prefix part of a word */
#define UPRO_SUFFIX_LEN 12

/** Total word length */
#define UPRO_WORD_LEN (UPRO_PREFIX_LEN + UPRO_SUFFIX_LEN)


/** Type to represent a nucleotide */
typedef int upro_nt;

/** Bits used to represent a nucleotide */
#define UPRO_NT_BITS 4

/** Nucleotide values */
enum {
    /** Adenine */
    UPRO_NT_A = (1 << 0),

    /** Cytosine */
    UPRO_NT_C = (1 << 1),

    /** Guanine */
    UPRO_NT_G = (1 << 2),

    /** Thymine */
    UPRO_NT_T = (1 << 3),

    /** Uracil */
    UPRO_NT_U = UPRO_NT_T,

    /** Wildcard matching any base */
    UPRO_NT_N = (UPRO_NT_A | UPRO_NT_C | UPRO_NT_G | UPRO_NT_T),

    /** Result of converting a non-alphabetic ([A-Za-z]) character */
    UPRO_NT_NOT_CHAR = -1,

    /** Result of converting a non-IUPAC symbol */
    UPRO_NT_NOT_IUPAC = -2,
};

/** Type used to represent a codon (or codon mask) */
typedef unsigned upro_codon;

/** Nucleotides in a codon */
#define UPRO_CODON_NTS 3

/** Bits used to represent a codon */
#define UPRO_CODON_BITS (UPRO_CODON_NTS * UPRO_NT_BITS)

/** Number of "real" codons */
#define UPRO_CODON_COUNT 64

/** Number of all possible binary representations of a codon or codon mask */
#define UPRO_BINARY_CODON_COUNT (1 << UPRO_CODON_BITS)


/** Type to represent one amino acid. */
typedef int upro_amino;

/** Bits needed to represent one amino acid */
#define UPRO_AMINO_BITS 5

/** Number of amino acids in the alphabet */
#define UPRO_ALPHABET_SIZE 20


/*----------*
 * PREFIXES *
 *----------*/

/** Type for prefixes
 *
 * Prefixes are (in contrast to suffixes, see below) contiguous, i.e. all values
 * from 0 to #UPRO_PREFIX_MAX represent valid prefixes.
 */
typedef uint_least32_t upro_prefix;
/** printf() format for prefixes */
#define UPRO_PREFIX_PRI PRIu32
/** scanf() format for prefixes */
#define UPRO_PREFIX_SCN SCNu32

/** Raise `x` to the power of 6 */
#define POW6(x) ((x) * (x) * (x) * (x) * (x) * (x))

/** Maximum value of a prefix */
#define UPRO_PREFIX_MAX (POW6((unsigned long) UPRO_ALPHABET_SIZE) - 1)


/*----------*
 * SUFFIXES *
 *----------*/

/** Type for suffixes
 *
 * Suffixes are represented as a "bit string" of #UPRO_SUFFIX_LEN amino acids,
 * each represented #UPRO_AMINO_BITS bits.
 */
typedef uint_least64_t upro_suffix;

/** printf() format for suffixes */
#define UPRO_SUFFIX_PRI PRIu64

/** scanf() format for suffixes */
#define UPRO_SUFFIX_SCN SCNu64


/** Identifier of a protein class (or "protein family"). */
typedef uint_least16_t upro_family;

/** Maximum value for #upro_family */
#define UPRO_FAMILY_MAX UINT_LEAST16_MAX

/** printf() format for #upro_family */
#define UPRO_FAMILY_PRI PRIu16

/** scanf() format for #upro_family */
#define UPRO_FAMILY_SCN SCNu16

#endif
