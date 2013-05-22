#ifndef EC_DISTMAT_H
#define EC_DISTMAT_H

/** \file ecurve/distmat.h
 *
 * Obtain alignment distances between amino acids
 *
 */

#include <stdio.h>
#include "ecurve/alphabet.h"

/** Alignment distance between two amino acids */
typedef double ec_dist;

/** printf() format for #ec_dist */
#define EC_DIST_PRI "g"

/** scanf() format for #ec_dist */
#define EC_DIST_SCN "lf"


/** Matrix of amino acid distances */
typedef struct ec_distmat_s ec_distmat;

/** Struct defining a distance matrix
 *
 * Applications should use the #ec_distmat typedef instead.
 */
struct ec_distmat_s {
    /** Matrix containing distances */
    ec_dist dists[EC_ALPHABET_SIZE << EC_AMINO_BITS];
};

/** Calculate index for given amino acids x, y
 *
 * Don't use this, it may change in the future. Use ec_distmat_get() and
 * ec_distmat_set().
 */
#define EC_DISTMAT_INDEX(x, y) ((x) << EC_AMINO_BITS | (y))


/** Initialize a distance matrix object to zeroes
 *
 * \param mat   distance matrix object to initialize
 */
int ec_distmat_init(ec_distmat *mat);

/** Get distance of two amino acids
 *
 * \details
 * Retrieves distance between two amino acids `x` and `y`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 *
 * \return  distance between x and y
 */
ec_dist ec_distmat_get(const ec_distmat *mat, ec_amino x, ec_amino y);

/** Set distance of two amino acids
 *
 * \details
 * Sets distance between `x` and `y` to `dist`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 * \param dist  distance between x and y
 */
void ec_distmat_set(ec_distmat *mat, ec_amino x, ec_amino y, ec_dist dist);

/** Load `n` distance matrices from a file, using the given loader function
 *
 * \details
 * \param mat   pointer to the first of `n` distance matrices
 * \param n     number of matrices to load
 * \param path  file to load
 * \param load  function that parses the file and populates the distance matrix
 * \param arg   additional argument to `load`
 */
int ec_distmat_load_many(ec_distmat *mat, size_t n, const char *path,
                         int (*load)(ec_distmat *, FILE *, void *), void *arg);

/** Load one distance matrices from a file, using the given loader function
 *
 * \details
 * \param mat   target distance matrix
 * \param path  file to load
 * \param load  function that parses the file and populates the distance matrix
 * \param arg   additional argument to `load`
 */
int ec_distmat_load(ec_distmat *mat, const char *path,
                    int (*load)(ec_distmat *, FILE *, void *), void *arg);

/** Read a BLOSUM-like distance matrix from a stream
 *
 * \details
 * BLOSUM matrices can be obtained from ftp://ftp.ncbi.nih.gov/blast/matrices/
 *
 * \param mat       distance matrix to populate
 * \param stream    input stream to parse (will NOT be closed after reading)
 * \param arg       pointer to an #ec_alphabet object to use for translating characters to amino acids
 */
int ec_distmat_load_blosum(ec_distmat *mat, FILE *stream, void *arg);

/** Read a plain text distance matrix from a stream
 *
 * \details
 *
 * Simply reads `#EC_ALPHABET_SIZE * #EC_ALPHABET_SIZE` numbers, separated by
 * white space, from the stream.
 *
 *
 * \param mat       distance matrix to populate
 * \param stream    input stream to parse (will NOT be closed after reading)
 * \param arg       (not used)
 */
int ec_distmat_load_plain(ec_distmat *mat, FILE *stream, void *arg);

#endif
