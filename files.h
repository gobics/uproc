#ifndef FILES_H
#define FILES_H

#include "uproc.h"

int db_load(const char *path, int proth_thresh_level,
            enum uproc_ecurve_format format,
            uproc_ecurve **fwd, uproc_ecurve **rev,
            uproc_idmap **idmap, uproc_matrix **prot_thresh);

int
model_load(const char *path, int orf_thresh_level, uproc_substmat **substmat,
           uproc_matrix **codon_scores, uproc_matrix **orf_thresh);
#endif
