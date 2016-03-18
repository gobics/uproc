/* Copyright 2014 Peter Meinicke, Robin Martinjak, Manuel Landesfeind
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

/** \file uproc/model.h
 *
 * Module: \ref grp_datafiles_model
 *
 * \weakgroup grp_datafiles
 * \{
 *
 * \weakgroup grp_datafiles_model
 * \{
 */

#ifndef UPROC_MODEL_H
#define UPROC_MODEL_H

#include "uproc/ecurve.h"
#include "uproc/idmap.h"
#include "uproc/substmat.h"
#include "uproc/matrix.h"

/** \defgroup obj_model object uproc_model
 *
 * Model
 *
 * \{
 */

/** \struct uproc_model
 * \copybrief obj_model
 *
 * See \ref obj_model for details.
 */
typedef struct uproc_model_s uproc_model;

/**
  * Loads all required data of a UProC model from files in the given directory
  *and returns a corresponding object.
  *
  * \param path             existing directory containing the UProC model files
  * \param orf_thresh_level ORF detection threshold to load, either 0, 1 or 2
  *
  * \returns the model object on success or %NULL on error
  */
uproc_model *uproc_model_load(const char *path, int orf_thresh_level);

/**
  * Returns the substitution matrix of the model. Note that the returned object
  *will
  * become invalid when the model is destroyed.
  *
  * \see uproc_model_destroy
  *
  * \param model the model
  * \returns a pointer to the matrix
  */
uproc_substmat *uproc_model_substitution_matrix(uproc_model *model);

/**
  * Returns the codon scores of the model. Note that the returned object will
  * become invalid when the model is destroyed.
  *
  * \see uproc_model_destroy
  *
  * \param model the model.
  * \returns a pointer to the ecurve.
  */
uproc_matrix *uproc_model_codon_scores(uproc_model *model);

/**
  * Returns the ORF threshold matrix of the model. Note that the returned object
  *will
  * become invalid when the model is destroyed.
  *
  * \see uproc_model_destroy
  *
  * \param model the model
  * \returns a pointer to the idmap.
  */
uproc_matrix *uproc_model_orf_threshold(uproc_model *model);

/**
  * Destroy the model and all associated objects.
  * \param model the model to destroy and free memory for.
  */
void uproc_model_destroy(uproc_model *model);

/** \} obj_model */

/**
 * \} grp_datafiles_model
 * \} grp_datafiles
 */
#endif
