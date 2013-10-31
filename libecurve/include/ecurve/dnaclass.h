#ifndef EC_DNACLASS_H
#define EC_DNACLASS_H

#include "ecurve/orf.h"

enum ec_dc_mode
{
    EC_DC_ALL,

    EC_DC_MAX,
};


struct ec_dc_results
{
    struct ec_dc_pred
    {
        ec_class cls;
        double score;
        unsigned frame;
    } *preds;
    size_t n, sz;
};

struct ec_dnaclass
{
    enum ec_dc_mode mode;

    const struct ec_protclass *pc;

    const struct ec_orf_codonscores *codon_scores;
    ec_orf_filter *orf_filter;
    void *orf_filter_arg;
};

int ec_dc_init(struct ec_dnaclass *dc,
        enum ec_dc_mode mode,
        const struct ec_protclass *pc,
        const struct ec_orf_codonscores *codon_scores,
        ec_orf_filter *orf_filter, void *orf_filter_arg);

int ec_dc_classify(const struct ec_dnaclass *dc, const char *seq,
        struct ec_dc_results *results);
#endif
