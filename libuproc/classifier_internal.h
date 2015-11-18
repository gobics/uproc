#ifndef UPROC_CLASSIFIER_INTERNAL_H
#define UPROC_CLASSIFIER_INTERNAL_H

#include <stdint.h>

#include "uproc/classifier.h"

uproc_clf *uproc_clf_create(
    enum uproc_clf_mode mode, void *context,
    int (*classify)(const void*, const char*, uproc_list*),
    void (*destroy)(void*));

#endif
