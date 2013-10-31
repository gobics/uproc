#ifndef UPRO_PROTCLASS_H
#define UPRO_PROTCLASS_H

/** \file upro/protclass.h
 *
 * Classify protein sequences
 */

#include "upro/common.h"
#include "upro/ecurve.h"
#include "upro/substmat.h"

typedef bool upro_pc_filter(const char*, size_t, upro_family, double, void*);

enum upro_pc_mode
{
    UPRO_PC_ALL,

    UPRO_PC_MAX,
};

struct upro_pc_results
{
    struct upro_pc_pred
    {
        upro_family family;
        double score;
    } *preds;
    size_t n, sz;
};

#define UPRO_PC_RESULTS_INITIALIZER { NULL, 0, 0 }

struct upro_protclass
{
    enum upro_pc_mode mode;
    const struct upro_substmat *substmat;
    const struct upro_ecurve *fwd;
    const struct upro_ecurve *rev;

    upro_pc_filter *filter;
    void *filter_arg;
};

int upro_pc_init(struct upro_protclass *pc,
        enum upro_pc_mode mode,
        const struct upro_ecurve *fwd,
        const struct upro_ecurve *rev,
        const struct upro_substmat *substmat,
        upro_pc_filter *filter, void *filter_arg);

void upro_pc_destroy(struct upro_protclass *pc);

int upro_pc_classify(const struct upro_protclass *pc, const char *seq,
        struct upro_pc_results *results);
#endif
