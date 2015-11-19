/* Classify sequences
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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
#include <math.h>

#include "uproc/classifier.h"
#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"
#include "uproc/list.h"
#include "uproc/mosaic.h"

#include "classifier_internal.h"

struct uproc_clf_s
{
    enum uproc_clf_mode mode;
    void *context;
    int (*classify)(const void *context, const char *seq, uproc_list *results);
    void (*destroy)(void *context);
};

uproc_clf *uproc_clf_create(
    enum uproc_clf_mode mode, void *context,
    int (*classify)(const void*, const char*, uproc_list*),
    void (*destroy)(void*))
{
    struct uproc_clf_s *clf = malloc(sizeof *clf);
    *clf = (struct uproc_clf_s) {
        .mode = mode,
            .context = context,
            .classify = classify,
            .destroy = destroy,
    };
    return clf;
}

void uproc_clf_destroy(uproc_clf *clf)
{
    uproc_assert(clf);
    uproc_assert(clf->destroy);
    clf->destroy(clf->context);
    free(clf);
}

static int result_cmpv(const void *p1, const void *p2)
{
    return uproc_result_cmp(p1, p2);
}

static int filter_results_to_max(uproc_list *results) {
    if (!uproc_list_size(results)) {
        return 0;
    }
    struct uproc_result shallow, deep;
    uproc_list_max(results, result_cmpv, &shallow);
    int res = uproc_result_copy(&deep, &shallow);
    if (res) {
        return res;
    }
    uproc_list_map2(results, uproc_result_freev);
    uproc_list_clear(results);
    return uproc_list_append(results, &deep);
}

int uproc_clf_classify(const uproc_clf *clf, const char *seq,
                       uproc_list **results)
{
    uproc_assert(clf);
    uproc_assert(clf->classify);
    uproc_assert(clf->destroy);


    if (!*results) {
        *results = uproc_list_create(sizeof (struct uproc_result));
        if (!*results) {
            return -1;
        }
    } else {
        uproc_list_map2(*results, uproc_result_freev);
        uproc_list_clear(*results);
    }

    int res = clf->classify(clf->context, seq, *results);
    if (res) {
        goto error;
    }

    if (clf->mode == UPROC_CLF_MAX) {
        res = filter_results_to_max(*results);
        if (res) {
            goto error;
        }
    }

    if (0) {
error:
        uproc_list_destroy(*results);
        *results = NULL;
        res = -1;
    }
    return res;
}

void uproc_result_init(struct uproc_result *result)
{
    *result = (struct uproc_result)UPROC_RESULT_INITIALIZER;
}

void uproc_result_free(struct uproc_result *result)
{
    uproc_orf_free(&result->orf);
    uproc_list_destroy(result->mosaicwords);
}

void uproc_result_freev(void *ptr)
{
    uproc_result_free(ptr);
}

int uproc_result_copy(struct uproc_result *dest,
                      const struct uproc_result *src)
{
    *dest = *src;
    if (src->orf.data) {
      int res = uproc_orf_copy(&dest->orf, &src->orf);
      if (res) {
          return res;
      }
    }
    if (!src->mosaicwords) {
        return 0;
    }
    dest->mosaicwords = uproc_list_create(sizeof (struct uproc_mosaicword));
    if (!dest->mosaicwords) {
        uproc_orf_free(&dest->orf);
        return -1;
    }
    return uproc_list_add(dest->mosaicwords, src->mosaicwords);
}

int uproc_result_cmp(const struct uproc_result *p1,
                     const struct uproc_result *p2)
{
    // lower rank is better
    int r = p2->rank - p1->rank;
    if (r) {
        return r;
    }
    // or higher score
    return ceil(p1->score - p2->score);
}
