/* A UProC model
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
#include "uproc/model.h"

#include "model_internal.h"

uproc_model *uproc_model_load(const char *path, int orf_thresh_level)
{
    uproc_model *m = malloc(sizeof *m);
    *m = (struct uproc_model_s)UPROC_MODEL_INITIALIZER;
    if (!m) {
        uproc_error_msg(UPROC_ENOMEM,
                        "can not allocate memory for model object");
        return NULL;
    }

    m->substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", path);
    if (!m->substmat) {
        goto error;
    }

    m->codon_scores = uproc_matrix_load(UPROC_IO_GZIP, "%s/codon_scores", path);
    if (!m->codon_scores) {
        goto error;
    }

    switch (orf_thresh_level) {
        case 1:
        case 2:
            m->orf_thresh = uproc_matrix_load(
                UPROC_IO_GZIP, "%s/orf_thresh_e%d", path, orf_thresh_level);
            if (!m->orf_thresh) {
                goto error;
            }
            break;

        case 0:
            break;

        default:
            uproc_error_msg(UPROC_EINVAL,
                            "ORF threshold level must be 0, 1, or 2");
            goto error;
    }

    return m;
error:
    uproc_model_destroy(m);
    return NULL;
}

void uproc_model_destroy(uproc_model *model)
{
    if (!model) {
        return;
    }

    if (model->substmat) {
        uproc_substmat_destroy(model->substmat);
        model->substmat = NULL;
    }
    if (model->codon_scores) {
        uproc_matrix_destroy(model->codon_scores);
        model->codon_scores = NULL;
    }
    if (model->orf_thresh) {
        uproc_matrix_destroy(model->orf_thresh);
        model->orf_thresh = NULL;
    }

    free(model);
}

uproc_substmat *uproc_model_substitution_matrix(uproc_model *model)
{
    if (!model) {
        uproc_error_msg(UPROC_EINVAL, "model parameter must not be NULL");
        return NULL;
    }
    return model->substmat;
}

uproc_matrix *uproc_model_codon_scores(uproc_model *model)
{
    if (!model) {
        uproc_error_msg(UPROC_EINVAL, "model parameter must not be NULL");
        return NULL;
    }
    return model->codon_scores;
}

uproc_matrix *uproc_model_orf_threshold(uproc_model *model)
{
    if (!model) {
        uproc_error_msg(UPROC_EINVAL, "model parameter must not be NULL");
        return NULL;
    }
    return model->orf_thresh;
}
