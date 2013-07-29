#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecurve/common.h"
#include "ecurve/alphabet.h"
#include "ecurve/substmat.h"

#define EC_DISTMAT_INDEX(x, y) ((x) << EC_AMINO_BITS | (y))

int
ec_substmat_init(struct ec_substmat *mat)
{
    *mat = (struct ec_substmat) { { 0.0 } };
    return EC_SUCCESS;
}

double
ec_substmat_get(const struct ec_substmat *mat, ec_amino x, ec_amino y)
{
    return mat->dists[EC_DISTMAT_INDEX(x, y)];
}

void
ec_substmat_set(struct ec_substmat *mat, ec_amino x, ec_amino y, double dist)
{
    mat->dists[EC_DISTMAT_INDEX(x, y)] = dist;
}

int
ec_substmat_load_many(struct ec_substmat *mat, size_t n, const char *path,
                     int (*load)(struct ec_substmat *, FILE *, void *), void *arg)
{
    int res = EC_SUCCESS;
    size_t i;
    FILE *stream;

    stream = fopen(path, "r");
    if (!stream) {
        return EC_FAILURE;
    }


    for (i = 0; i < n; i++) {
        res = ec_substmat_init(&mat[i]);
        if (res != EC_SUCCESS) {
            goto error;
        }
        res = (*load)(&mat[i], stream, arg);
        if (res != EC_SUCCESS) {
            goto error;
        }
    }

error:
    fclose(stream);
    return res;
}

int
ec_substmat_load(struct ec_substmat *mat, const char *path,
                int (*load)(struct ec_substmat *, FILE *, void *), void *arg)
{
    return ec_substmat_load_many(mat, 1, path, load, arg);
}

int
ec_substmat_load_blosum(struct ec_substmat *mat, FILE *stream, void *arg)
{
    int i, n, row[EC_ALPHABET_SIZE];
    ec_amino aa[EC_ALPHABET_SIZE];
    char line[1024];
    struct ec_alphabet *alpha = arg;

    /* skip comments plus header line (which begins with two spaces) */
    do {
        if (!fgets(line, sizeof line, stream)) {
            return EC_FAILURE;
        }
    } while (strncmp(line, "  ", 2));

    for (n = i = 0; n < EC_ALPHABET_SIZE; i++) {
        int k, r;
        char *p;

        if (!fgets(line, sizeof line, stream)) {
            return EC_FAILURE;
        }

        aa[n] = ec_alphabet_char_to_amino(alpha, *line);
        if (aa[n] < 0) {
            continue;
        }
        row[n] = i;

        for (p = line + 1, r = 0, k = 0; k <= n; k++) {
            double dist;
            do {
                dist = strtod(p, &p);
            } while (row[k] != r++);
            ec_substmat_set(mat, aa[n], aa[k], dist);
            ec_substmat_set(mat, aa[k], aa[n], dist);
        }
        n += 1;
    }

    return EC_SUCCESS;
}

int
ec_substmat_load_plain(struct ec_substmat *mat, FILE *stream, void *arg)
{
    size_t i, k;
    double dist;
    (void) arg;
    for (i = 0; i < EC_ALPHABET_SIZE; i++) {
        for (k = 0; k < EC_ALPHABET_SIZE; k++) {
            if (fscanf(stream, "%lf", &dist) != 1) {
                return EC_FAILURE;
            }
            ec_substmat_set(mat, i, k, dist);
        }
    }
    return 0;
}
