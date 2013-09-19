#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mat.h>

#define ALPHABET "ATSPGNDEQKRHYWFMLIVC"

#include "ecurve.h"

int main(int argc, char **argv)
{
    MATFile *f;
    ec_prefix p;
    size_t last_p;
    ec_suffix s;
    mxArray *mInd, *mSuffixes, *mClasses;
    uint32_t *ind;
    uint64_t *suffixes;
    uint16_t *classes;
    size_t suffix_count;

    struct ec_ecurve ec;

    if (argc != 3) {
        fprintf(stderr, "usage: %s INFILE OUTFILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "opening file...\n");
    f = matOpen(argv[1], "r");
    if (!f) {
        fprintf(stderr, "cannot open file\n");
        perror("");
        return EXIT_FAILURE;
    }
    //ind_up = matGetVariable(f, "HexIndsUp");
    mSuffixes = matGetVariable(f, "EcurveWords");
    fprintf(stderr, "type of EcurveWords: %s\n", mxGetClassName(mSuffixes));
    mClasses = matGetVariable(f, "EcurvePfams");
    fprintf(stderr, "type of EcurvePfams: %s\n", mxGetClassName(mClasses));

    suffixes = mxGetData(mSuffixes);
    classes = mxGetData(mClasses);

    suffix_count = mxGetM(mSuffixes);
    ec_ecurve_init(&ec, ALPHABET, suffix_count);
    for (s = 0; s < suffix_count; s++) {
        ec.suffixes[s] = suffixes[s];
        ec.classes[s] = classes[s];
    }
    mxDestroyArray(mSuffixes);
    mxDestroyArray(mClasses);

    mInd = matGetVariable(f, "HexIndsLo");
    ind = mxGetData(mInd);
    fprintf(stderr, "type of HexIndsLo: %s\n", mxGetClassName(mInd));
    for (last_p = p = 0; p <= EC_PREFIX_MAX; p++) {
        size_t count;
        if (p == EC_PREFIX_MAX) {
            count = suffix_count - ind[p];
        }
        else {
            count = ind[p + 1] - ind[p];
            if (!last_p && count) {
                count++;
            }
        }

        ec.prefixes[p].first = last_p;

        if (!count) {
            ec.prefixes[p].count = (last_p) ? 0 : -1;
        }
        else if (last_p >= suffix_count) {
            ec.prefixes[p].count = -1;
        }
        else {
            ec.prefixes[p].count = count;
            last_p += count;
        }
    }
    mxDestroyArray(mInd);

    ec_storage_store_file(&ec, argv[2], EC_STORAGE_PLAIN);

    matClose(f);
    return EXIT_SUCCESS;
}
