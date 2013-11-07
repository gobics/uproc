#ifndef UPRO_DNACLASS_H
#define UPRO_DNACLASS_H

#include "upro/orf.h"
#include "upro/protclass.h"
#include "upro/matrix.h"

/** \file upro/dnaclass.h
 *
 * Classify DNA/RNA sequences
 */
enum upro_dc_mode
{
    UPRO_DC_ALL,

    UPRO_DC_MAX,
};

struct upro_dc_results
{
    struct upro_dc_pred
    {
        upro_family family;
        double score;
        unsigned frame;
    } *preds;
    size_t n, sz;
};

#define UPRO_DC_RESULTS_INITIALIZER { NULL, 0, 0 }

struct upro_dnaclass
{
    enum upro_dc_mode mode;

    const struct upro_protclass *pc;

    double codon_scores[UPRO_BINARY_CODON_COUNT];
    upro_orf_filter *orf_filter;
    void *orf_filter_arg;
};

int upro_dc_init(struct upro_dnaclass *dc,
        enum upro_dc_mode mode,
        const struct upro_protclass *pc,
        const struct upro_matrix *codon_scores,
        upro_orf_filter *orf_filter, void *orf_filter_arg);

int upro_dc_classify(const struct upro_dnaclass *dc, const char *seq,
        struct upro_dc_results *results);
#endif