#ifndef EC_COMMON_H
#define EC_COMMON_H

/** \file ecurve/common.h
 *
 * Common definitions
 */

#include <stdint.h>
#include <inttypes.h>
#include <math.h>

/** Return codes */
enum {
    /** Success! */
    EC_SUCCESS = 0,

    /** General failure */
    EC_FAILURE,

    /** Iterator produced an item */
    EC_ITER_YIELD,

    /** Iterator exhausted */
    EC_ITER_STOP,

    /** Memory allocation failed */
    EC_ENOMEM,

    /** Invalid argument */
    EC_EINVAL,

    /** Object doesn't exist  */
    EC_ENOENT,

    /** Object already exists */
    EC_EEXIST,

    /** A system call (that sets `errno`) returned an error */
    EC_ESYSCALL,
};

/** Test return code for error */
#define EC_ISERROR(e) ((e) != EC_SUCCESS && (e) != EC_ITER_YIELD && (e) != EC_ITER_STOP)

/** Epsilon value for comparing floating point numbers */
#define EC_EPSILON 1e-5

/** Lowest `n` bits set to one */
#define EC_BITMASK(n) (~(~0ULL << (n)))

/** Length of the suffix part of a word */
#define EC_PREFIX_LEN 6

/** Length of the prefix part of a word */
#define EC_SUFFIX_LEN 12

/** Total word length */
#define EC_WORD_LEN (EC_PREFIX_LEN + EC_SUFFIX_LEN)


/** Type to represent a nucleotide */
typedef int ec_nt;

/** Bits used to represent a nucleotide */
#define EC_NT_BITS 4

/** Nucleotide values */
enum {
    /** Adenine */
    EC_NT_A = (1 << 0),

    /** Cytosine */
    EC_NT_C = (1 << 1),

    /** Guanine */
    EC_NT_G = (1 << 2),

    /** Thymine */
    EC_NT_T = (1 << 3),

    /** Uracil */
    EC_NT_U = EC_NT_T,

    /** Wildcard matching any base */
    EC_NT_N = (EC_NT_A | EC_NT_C | EC_NT_G | EC_NT_T),

    /** Result of converting a non-alphabetic ([A-Za-z]) character */
    EC_NT_NOT_CHAR = -1,

    /** Result of converting a non-IUPAC symbol */
    EC_NT_NOT_IUPAC = -2,
};

/** Type used to represent a codon (or codon mask) */
typedef unsigned ec_codon;

/** Nucleotides in a codon */
#define EC_CODON_NTS 3

/** Bits used to represent a codon */
#define EC_CODON_BITS (EC_CODON_NTS * EC_NT_BITS)

/** Number of "real" codons */
#define EC_CODON_COUNT 64

/** Number of all possible binary representations of a codon or codon mask */
#define EC_BINARY_CODON_COUNT (1 << EC_CODON_BITS)


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
typedef uint_least32_t ec_prefix;
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
typedef uint_least64_t ec_suffix;

/** printf() format for suffixes */
#define EC_SUFFIX_PRI PRIu64

/** scanf() format for suffixes */
#define EC_SUFFIX_SCN SCNu64


/** Identifier of a protein class (or "protein family"). */
typedef uint_least16_t ec_class;

/** printf() format for #ec_class */
#define EC_CLASS_PRI PRIu16

/** scanf() format for #ec_class */
#define EC_CLASS_SCN SCNu16

#endif
