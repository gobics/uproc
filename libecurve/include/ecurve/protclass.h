#ifndef EC_PROTCLASS_H
#define EC_PROTCLASS_H

/** \file ecurve/protclass.h
 *
 * Classify protein sequences
 */

#include "ecurve/common.h"
#include "ecurve/ecurve.h"
#include "ecurve/substmat.h"

typedef bool ec_pc_filter(const char*, size_t, ec_class, double, void*);

enum ec_pc_mode
{
    EC_PC_ALL,

    EC_PC_MAX,
};

struct ec_pc_results
{
    struct ec_pc_pred
    {
        ec_class cls;
        double score;
    } *preds;
    size_t n, sz;
};

#define EC_PC_RESULTS_INITIALIZER { NULL, 0, 0 }

struct ec_protclass
{
    enum ec_pc_mode mode;
    const struct ec_substmat *substmat;
    const struct ec_ecurve *fwd;
    const struct ec_ecurve *rev;

    ec_pc_filter *filter;
    void *filter_arg;
};

int ec_pc_init(struct ec_protclass *pc,
        enum ec_pc_mode mode,
        const struct ec_ecurve *fwd,
        const struct ec_ecurve *rev,
        const struct ec_substmat *substmat,
        ec_pc_filter *filter, void *filter_arg);

void ec_pc_destroy(struct ec_protclass *pc);

int ec_pc_classify(const struct ec_protclass *pc, const char *seq,
        struct ec_pc_results *results);
#endif
