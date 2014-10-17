/* A database containing ecurves
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak, Manuel Landesfeind
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "uproc/error.h"
#include "uproc/database.h"

#include "database_internal.h"

uproc_database * 
uproc_database_load(const char *path, int prot_thresh_level,
              enum uproc_ecurve_format format)
{
	uproc_database *db = malloc(sizeof(uproc_database));
	if( ! db ){
		uproc_error_msg(UPROC_ENOMEM, "can not allocate memory for database object");
		return NULL;
	}
	
    switch (prot_thresh_level) {
        case 2:
        case 3:
            db->prot_thresh = uproc_matrix_load(
                UPROC_IO_GZIP, "%s/prot_thresh_e%d", path, prot_thresh_level);
            if (!db->prot_thresh) {
                goto error;
            }
            break;

        case 0:
            break;

        default:
            uproc_error_msg(
                UPROC_EINVAL, "protein threshold level must be 0, 2, or 3");
            goto error;
    }

    db->idmap = uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap", path);
    if (!db->idmap) {
        goto error;
    }
    db->fwd = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/fwd.ecurve", path);
    if (!db->fwd) {
        goto error;
    }
    db->rev = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/rev.ecurve", path);
    if (!db->rev) {
        goto error;
    }

    return db;
error:
    uproc_database_destroy(db);
    return NULL;
}


void uproc_database_destroy(uproc_database *db){
	if( ! db ){
		return;
	}	
	
	if( db->idmap ){
		uproc_idmap_destroy(db->idmap);
		db->idmap = NULL;
	}
	if( db->fwd ){
		uproc_ecurve_destroy(db->fwd);
		db->fwd = NULL;
	}
	if( db->rev ){
		uproc_ecurve_destroy(db->rev);
		db->rev = NULL;
	}
	if( db->prot_thresh ){
		uproc_matrix_destroy(db->prot_thresh);
		db->prot_thresh = NULL;
	}

	free(db);
}
	
uproc_ecurve *uproc_database_ecurve_forward(uproc_database *db){
	if( ! db ){
		uproc_error_msg(UPROC_EINVAL, "database parameter must not be NULL");
		return NULL;
	}	
	return db->fwd;
}
	
uproc_ecurve *uproc_database_ecurve_reverse(uproc_database *db){
	if( ! db ){
		uproc_error_msg(UPROC_EINVAL, "database parameter must not be NULL");
		return NULL;
	}	
	return db->rev;
}
uproc_idmap  *uproc_database_idmap(uproc_database *db){
	if( ! db ){
		uproc_error_msg(UPROC_EINVAL, "database parameter must not be NULL");
		return NULL;
	}	
	return db->idmap;
}
uproc_matrix *uproc_database_protein_threshold(uproc_database *db){
	if( ! db ){
		uproc_error_msg(UPROC_EINVAL, "database parameter must not be NULL");
		return NULL;
	}	
	return db->prot_thresh;
}

