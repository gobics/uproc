#ifndef UPRO_SUBSTMAT_H
#define UPRO_SUBSTMAT_H

/** \file upro/substmat.h
 *
 * Obtain alignment distances between amino acids
 *
 */

#include <stdio.h>
#include <math.h>
#include "upro/common.h"
#include "upro/io.h"
#include "upro/alphabet.h"

/** Matrix of amino acid distances */
struct upro_substmat {
    /** Matrix containing distances */
    double dists[UPRO_ALPHABET_SIZE << UPRO_AMINO_BITS];
};

/** Calculate index for given amino acids x, y
 *
 * Don't use this, it may change in the future. Use upro_substmat_get() and
 * upro_substmat_set().
 */
#define UPRO_SUBSTMAT_INDEX(x, y) ((x) << UPRO_AMINO_BITS | (y))


/** Initialize a distance matrix object to zeroes
 *
 * \param mat   distance matrix object to initialize
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_substmat_init(struct upro_substmat *mat);

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
double upro_substmat_get(const struct upro_substmat *mat, upro_amino x, upro_amino y);

/** Set distance of two amino acids
 *
 * Sets distance between `x` and `y` to `dist`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 * \param dist  distance between x and y
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
void upro_substmat_set(struct upro_substmat *mat, upro_amino x, upro_amino y,
                    double dist);

/** Load `n` distance matrices from a file, using the given loader function
 *
 * \param mat   pointer to the first of `n` distance matrices
 * \param n     number of matrices to load
 * \param path  file to load
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_substmat_load_many(struct upro_substmat *mat, size_t n, const char *path,
        enum upro_io_type iotype);

/** Load one distance matrices from a file, using the given loader function
 *
 * \param mat   target distance matrix
 * \param path  file to load
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_substmat_load(struct upro_substmat *mat, const char *path,
        enum upro_io_type iotype);

#endif
