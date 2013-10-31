#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mat.h>

#define ALPHABET "ATSPGNDEQKRHYWFMLIVC"

#include "upro.h"

int main(int argc, char **argv)
{
    MATFile *f;
    upro_prefix p;
    size_t last_p;
    upro_suffix s;
    mxArray *mInd, *mSuffixes, *mClasses;
    uint32_t *ind;
    uint64_t *suffixes;
    uint16_t *families;
    size_t suffix_count;

    struct upro_ecurve ec;

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
    families = mxGetData(mClasses);

    suffix_count = mxGetM(mSuffixes);
    upro_ecurve_init(&ec, ALPHABET, suffix_count);
    for (s = 0; s < suffix_count; s++) {
        ec.suffixes[s] = suffixes[s];
        ec.families[s] = families[s];
    }
    mxDestroyArray(mSuffixes);
    mxDestroyArray(mClasses);

    mInd = matGetVariable(f, "HexIndsLo");
    ind = mxGetData(mInd);
    fprintf(stderr, "type of HexIndsLo: %s\n", mxGetClassName(mInd));
    for (last_p = p = 0; p <= UPRO_PREFIX_MAX; p++) {
        size_t count;
        if (p == UPRO_PREFIX_MAX) {
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

    upro_storage_store(&ec, argv[2], UPRO_STORAGE_PLAIN, UPRO_IO_GZIP);

    matClose(f);
    return EXIT_SUCCESS;
}
