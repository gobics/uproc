#ifndef UPROC_DATABASE_INTERNAL_H
#define UPROC_DATABASE_INTERNAL_H

#include "uproc/ecurve.h"
#include "uproc/matrix.h"
#include "uproc/idmap.h"

/** Struct defining a database **/
struct uproc_database_s
{
    /**
      * The forward matching ecurve.
      * \see uproc_ecurve
      */
    uproc_ecurve *fwd;
    /**
      * The backward matching ecurve.
      * \see uproc_ecurve
      */
    uproc_ecurve *rev;
    /**
      * The mapping of numerical ID to string ID.
      */
    uproc_idmap *idmap;
    /**
      * The matrix containing protein thresholds.
      */
    uproc_matrix *prot_thresh;
};

/**
  * Initializer for the database struct setting all pointers to %NULL.
  */
#define UPROC_DATABASE_INITIALIZER \
    {                              \
        0, 0, 0, 0                 \
    }

#endif
