#ifndef EC_SUBSTMAT_H
#define EC_SUBSTMAT_H

/** \file ecurve/substmat.h
 *
 * Obtain alignment distances between amino acids
 *
 */

#include <stdio.h>
#include <math.h>
#include "ecurve/common.h"
#include "ecurve/io.h"
#include "ecurve/alphabet.h"

/** Matrix of amino acid distances */
struct ec_substmat {
    /** Matrix containing distances */
    double dists[EC_ALPHABET_SIZE << EC_AMINO_BITS];
};

/** Calculate index for given amino acids x, y
 *
 * Don't use this, it may change in the future. Use ec_substmat_get() and
 * ec_substmat_set().
 */
#define EC_SUBSTMAT_INDEX(x, y) ((x) << EC_AMINO_BITS | (y))


/** Initialize a distance matrix object to zeroes
 *
 * \param mat   distance matrix object to initialize
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_substmat_init(struct ec_substmat *mat);

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
double ec_substmat_get(const struct ec_substmat *mat, ec_amino x, ec_amino y);

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
void ec_substmat_set(struct ec_substmat *mat, ec_amino x, ec_amino y,
                    double dist);

/** Load `n` distance matrices from a file, using the given loader function
 *
 * \param mat   pointer to the first of `n` distance matrices
 * \param n     number of matrices to load
 * \param path  file to load
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_substmat_load_many(struct ec_substmat *mat, size_t n, const char *path,
        enum ec_io_type iotype);

/** Load one distance matrices from a file, using the given loader function
 *
 * \param mat   target distance matrix
 * \param path  file to load
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_substmat_load(struct ec_substmat *mat, const char *path,
        enum ec_io_type iotype);

#endif
