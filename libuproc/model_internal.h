#ifndef UPROC_DATABASE_INTERNAL_H
#define UPROC_DATABASE_INTERNAL_H

#include "uproc/substmat.h"
#include "uproc/matrix.h"

/* Struct representing the "model" */
struct uproc_model_s
{
    uproc_substmat *substmat;
    uproc_matrix *codon_scores, *orf_thresh;
};

#define UPROC_MODEL_INITIALIZER { 0, 0, 0 }

#endif
