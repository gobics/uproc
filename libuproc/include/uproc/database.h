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

/** \file uproc/database.h
 *
 * Module: \ref grp_datastructs_database
 *
 * \weakgroup grp_datastructs
 * \{
 *
 * \weakgroup grp_datastructs_database
 * \{
 */

#ifndef UPROC_DATABASE_H
#define UPROC_DATABASE_H

#include "uproc/ecurve.h"
#include "uproc/dict.h"
#include "uproc/idmap.h"
#include "uproc/matrix.h"

/** \defgroup obj_database object uproc_database
 *
 * Database
 *
 * \{
 */

/** \struct uproc_database
 * \copybrief obj_database
 *
 * See \ref obj_database for details.
 */
typedef struct uproc_database_s uproc_database;

uproc_database *uproc_database_create(void);

/** Load database from directory.
 *
 * Loads all required data of a UProC database from files in * the given
 * directory and returns a database object.
 *
 * \param path  existing directory containing a UProC database
 * \returns the object on success or %NULL on error
 */
uproc_database *uproc_database_load(const char *path);

/** Flags for :uproc_database_load_some */
enum uproc_database_load_which {
    /** Load the metadata dict */
    UPROC_DATABASE_LOAD_METADATA = 1,

    /** Load protein threshold matrices */
    UPROC_DATABASE_LOAD_PROT_THRESH = (1 << 1),

    /** Load class ID to string maps */
    UPROC_DATABASE_LOAD_IDMAPS = (1 << 2),

    /** Load ecurves */
    UPROC_DATABASE_LOAD_ECURVES = (1 << 3),

    /** Load everything */
    UPROC_DATABASE_LOAD_ALL = 0xff,
};

/** Load some parts of a database from directory. */
uproc_database *uproc_database_load_some(const char *path, int which);

/** Store database to directory.
 *
 * Stores all database elements to their corresponding files in the directory
 * given by \c path. This directory must already exist.
 * When storing the ecurves, \c progress is periodically called with the
 * current progress in percent.
 */
int uproc_database_store(const uproc_database *db, const char *path,
                         void (*progress)(double));

/** Get the forward matching ecurve of the database.
 *
 * The database retains ownership of the returned object.
 */
uproc_ecurve *uproc_database_ecurve_forward(uproc_database *db);

/** Set forward ecurve.
 *
 * The database takes ownership of \c ecurve
 */
void uproc_database_set_ecurve_forward(uproc_database *db,
                                       uproc_ecurve *ecurve);

/** Get the reverse matching ecurve of the database.
 *
 * The database retains ownership of the returned object.
 */
uproc_ecurve *uproc_database_ecurve_reverse(uproc_database *db);

/** Set reverse ecurve.
 *
 * The database takes ownership of \c ecurve
 */
void uproc_database_set_ecurve_reverse(uproc_database *db,
                                       uproc_ecurve *ecurve);

/** Returns the mapping from numerical to string IDs of the database
 *
 * The database retains ownership of the returned object.
 */
uproc_idmap *uproc_database_idmap(uproc_database *db, uproc_rank rank);

/** Set idmap
 *
 * The database takes ownership of \c idmap
 */
void uproc_database_set_idmap(uproc_database *db, uproc_rank rank,
                              uproc_idmap *idmap);

/** Return the protein threshold matrix for the given level.
 *
 * Valid levels are 0, 2 and 3. If \c level is 0, returns NULL.
 * The database retains ownership of the returned object.
 */
uproc_matrix *uproc_database_protein_threshold(uproc_database *db, int level);

/** Set protein threshold matrix
 *
 * Valid levels are 2 and 3.
 *
 * The database takes ownership of \c prot_thresh
 */
int uproc_database_set_protein_threshold(uproc_database *db, int level,
                                         uproc_matrix *prot_thresh);

uproc_alphabet *uproc_database_alphabet(uproc_database *db);

/**
 * Destroy the database and all associated object within the database.
 * \param db the database to destroy and free memory for.
 */
void uproc_database_destroy(uproc_database *db);

/** \} */

// The size of the largest primitive type in the union corresponds to its
// alignment (unless the enum type would be larger, but that's not possible
// with uintmax_t in the union)
#define UPROC_DATABASE_METADATA_STR_SIZE \
    (UPROC_DICT_VALUE_SIZE_MAX - sizeof(uintmax_t))
struct uproc_database_metadata
{
    enum uproc_database_metadata_type {
        STR = 's',
        UINT = 'u',
    } type;
    union
    {
        char str[UPROC_DATABASE_METADATA_STR_SIZE];
        uintmax_t uint;
    } value;
};

int uproc_database_metadata_get_uint(const uproc_database *db, const char *key,
                                     uintmax_t *value);

int uproc_database_metadata_get_str(
    const uproc_database *db, const char *key,
    char value[static UPROC_DATABASE_METADATA_STR_SIZE]);

int uproc_database_metadata_set_uint(uproc_database *db, const char *key,
                                     uintmax_t value);

int uproc_database_metadata_set_str(uproc_database *db, const char *key,
                                    char *value);
#endif
