#ifndef EC_DISTMAT_H
#define EC_DISTMAT_H

/** \file ecurve/distmat.h
 *
 * Obtain alignment distances between amino acids
 *
 */

#include <stdio.h>
#include <math.h>
#include "ecurve/common.h"
#include "ecurve/alphabet.h"

/** Matrix of amino acid distances */
struct ec_distmat {
    /** Matrix containing distances */
    double dists[EC_ALPHABET_SIZE << EC_AMINO_BITS];
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
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_distmat_init(struct ec_distmat *mat);

/** Get distance of two amino acids
 *
 * Retrieves distance between two amino acids `x` and `y`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 *
 * \return  distance between x and y
 */
double ec_distmat_get(const struct ec_distmat *mat, ec_amino x, ec_amino y);

/** Set distance of two amino acids
 *
 * Sets distance between `x` and `y` to `dist`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 * \param dist  distance between x and y
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
void ec_distmat_set(struct ec_distmat *mat, ec_amino x, ec_amino y,
                    double dist);

/** Load `n` distance matrices from a file, using the given loader function
 *
 * \param mat   pointer to the first of `n` distance matrices
 * \param n     number of matrices to load
 * \param path  file to load
 * \param load  function that parses the file and populates the distance matrix
 * \param arg   additional argument to `load`
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_distmat_load_many(struct ec_distmat *mat, size_t n, const char *path,
                         int (*load)(struct ec_distmat *, FILE *, void *),
                         void *arg);

/** Load one distance matrices from a file, using the given loader function
 *
 * \param mat   target distance matrix
 * \param path  file to load
 * \param load  function that parses the file and populates the distance matrix
 * \param arg   additional argument to `load`
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_distmat_load(struct ec_distmat *mat, const char *path,
                    int (*load)(struct ec_distmat *, FILE *, void *),
                    void *arg);

/** Read a BLOSUM-like distance matrix from a stream
 *
 * BLOSUM matrices can be obtained from ftp://ftp.ncbi.nih.gov/blast/matrices/
 *
 * \param mat       distance matrix to populate
 * \param stream    input stream to parse (will NOT be closed after reading)
 * \param arg       pointer to an #ec_alphabet object to use for translating characters to amino acids
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_distmat_load_blosum(struct ec_distmat *mat, FILE *stream, void *arg);

/** Read a plain text distance matrix from a stream
 *
 * Simply reads `#EC_ALPHABET_SIZE * #EC_ALPHABET_SIZE` numbers, separated by
 * white space, from the stream.
 *
 * \param mat       distance matrix to populate
 * \param stream    input stream to parse (will NOT be closed after reading)
 * \param arg       (not used)
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_distmat_load_plain(struct ec_distmat *mat, FILE *stream, void *arg);

#endif
