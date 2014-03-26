#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "files.h"
#include "uproc.h"

int
db_load(const char *path, int prot_thresh_level,
        enum uproc_ecurve_format format,
        uproc_ecurve **fwd, uproc_ecurve **rev,
        uproc_idmap **idmap, uproc_matrix **prot_thresh)
{
    *fwd = *rev = NULL;
    *idmap = NULL;
    *prot_thresh = NULL;

    switch (prot_thresh_level) {
        case 2:
        case 3:
            *prot_thresh = uproc_matrix_load(
                UPROC_IO_GZIP, "%s/prot_thresh_e%d", path, prot_thresh_level);
            if (!*prot_thresh) {
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

    *idmap = uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap", path);
    if (!*idmap) {
        goto error;
    }
    *fwd = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/fwd.ecurve", path);
    if (!*fwd) {
        goto error;
    }
    *rev = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/rev.ecurve", path);
    if (!*rev) {
        goto error;
    }

    return 0;
error:
    uproc_ecurve_destroy(*fwd);
    uproc_ecurve_destroy(*rev);
    uproc_idmap_destroy(*idmap);
    uproc_matrix_destroy(*prot_thresh);
    return -1;
}


int
model_load(const char *path, int orf_thresh_level, uproc_substmat **substmat,
           uproc_matrix **codon_scores, uproc_matrix **orf_thresh)
{
    *substmat = NULL;
    *codon_scores = *orf_thresh = NULL;

    *substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", path);
    if (!*substmat) {
        goto error;
    }

    *codon_scores = uproc_matrix_load(UPROC_IO_GZIP, "%s/codon_scores",
                                      path);
    if (!*codon_scores) {
        goto error;
    }

    switch (orf_thresh_level) {
        case 1:
        case 2:
            *orf_thresh = uproc_matrix_load(UPROC_IO_GZIP, "%s/orf_thresh_e%d",
                                            path, orf_thresh_level);
            if (!*orf_thresh) {
                goto error;
            }
            break;

        case 0:
            break;

        default:
            uproc_error_msg(
                UPROC_EINVAL, "ORF threshold level must be 0, 1, or 2");
            goto error;
    }

    return 0;
error:
    uproc_substmat_destroy(*substmat);
    uproc_matrix_destroy(*codon_scores);
    uproc_matrix_destroy(*orf_thresh);
    return -1;
}
