/* Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file uproc/mosaic.h
 *
 * Module: \ref grp_intern_mosaic
 *
 * \weakgroup grp_intern
 * \{
 *
 * \weakgroup grp_intern_mosaic
 * \{
 */

#ifndef UPROC_MOSAIC_H
#define UPROC_MOSAIC_H

#include <stdbool.h>
#include <stdlib.h>

#include "uproc/common.h"
#include "uproc/ecurve.h"
#include "uproc/list.h"
#include "uproc/word.h"

/** \defgroup struct_mosaicword struct uproc_mosaicword
 *
 * Amino acid word found in an ecurve.
 *
 * This struct contains information about what was found while looking up
 * a word in an ecurve.
 *
 */

struct uproc_mosaicword {
    /** Found word (probably different from the looked up word). */
    struct uproc_word word;

    /** Position of the looked up word in the input sequence this word. */
    size_t index;

    /** Whether this word matched in the forward or reverse ecurve */
    enum uproc_ecurve_direction dir;

    /** Score of this single word.
     *
     * This is the sum of all positional scores and will likely not reflect
     * what the word "contributed" to the overall mosaic score.
     */
    double score;
};

/** Initializes a ::uproc_mosaicword struct from the given values. */
void uproc_mosaicword_init(struct uproc_mosaicword *mw,
                           const struct uproc_word *word,
                           size_t index,
                           double dist[static UPROC_SUFFIX_LEN],
                           enum uproc_ecurve_direction dir);
/** \} */

/** \defgroup obj_mosaic object uproc_mosaic
 *
 * AA word mosaic
 *
 * This object is used to calculate the mosaic score of an aminoacid sequence.
 * Instead of a whole matrix, only UPROC_WORD_LEN positional scores are stored.
 * As a result, only O(1) storage is required and there is no need to reallocate
 * memory when words are added.
 * As a consequence, words _must_ be added in their order of appearance in the
 * sequence.
 * Optionally, a mosaic object can be instructed to store all added words in a
 * list (see \ref struct_mosaicword), which is of course less efficient.
 */

/** \struct uproc_mosaic
 * \copybrief obj_mosaic
 *
 * See \ref obj_mosaic for details.
 */
typedef struct uproc_mosaic_s uproc_mosaic;

/** Create mosaic object
 *
 * \param store_words   whether to build a list of all added words
 */
uproc_mosaic *uproc_mosaic_create(bool store_words);

/** Destroy mosaic object */
void uproc_mosaic_destroy(uproc_mosaic *m);

/** Add word to mosaic */
int uproc_mosaic_add(uproc_mosaic *m, const struct uproc_word *w, size_t index,
                     double dist[static UPROC_SUFFIX_LEN],
                     enum uproc_ecurve_direction dir);

/** Get final score
 *
 * Adds the remaining positional scores to the total and returns it.
 *
 * \note
 * ProTip: a mosaic object is returned to its initial state after a call to
 * ::uproc_mosaic_finalize (and ::uproc_mosaic_words_mv if it was created with
 * \c store_words set to true) and can be reused.
 *
 * \param m     mosaic object
 * \returns total mosaic score
 */
double uproc_mosaic_finalize(uproc_mosaic *m);

/** Get mosaicword list (with ownership)
 *
 * Returns the list of all added words and sets the mosaics internal pointer to
 * NULL (i.e. ownership is transferred to the caller).
 */
uproc_list *uproc_mosaic_words_mv(uproc_mosaic *m);
#endif
