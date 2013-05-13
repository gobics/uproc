#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecurve/common.h"
#include "ecurve/alphabet.h"
#include "ecurve/distmat.h"

#define EC_DISTMAT_INDEX(x, y) ((x) << EC_AMINO_BITS | (y))
#define EC_DIST_SCANFMT "%lf"

int
ec_distmat_init(ec_distmat *mat)
{
    struct ec_distmat_s *m = mat;
    *m = (struct ec_distmat_s) { { 0.0 } };
    return EC_SUCCESS;
}

ec_dist
ec_distmat_get(const ec_distmat *mat, ec_amino x, ec_amino y)
{
    const struct ec_distmat_s *m = mat;
    return m->dists[EC_DISTMAT_INDEX(x, y)];
}

void
ec_distmat_set(ec_distmat *mat, ec_amino x, ec_amino y, ec_dist dist)
{
    struct ec_distmat_s *m = mat;
    m->dists[EC_DISTMAT_INDEX(x, y)] = m->dists[EC_DISTMAT_INDEX(y, x)] = dist;
}

int
ec_distmat_load_many(ec_distmat *mat, size_t n, const char *path,
                     int (*load)(ec_distmat *, FILE *, void *), void *arg)
{
    int res = EC_SUCCESS;
    size_t i;
    FILE *stream;

    stream = fopen(path, "r");
    if (!stream) {
        return EC_FAILURE;
    }


    for (i = 0; i < n; i++) {
        res = ec_distmat_init(&mat[i]);
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
ec_distmat_load(ec_distmat *mat, const char *path,
                int (*load)(ec_distmat *, FILE *, void *), void *arg)
{
    return ec_distmat_load_many(mat, 1, path, load, arg);
}

int
ec_distmat_load_blosum(ec_distmat *mat, FILE *stream, void *arg)
{
    int i, n, row[EC_ALPHABET_SIZE];
    ec_amino aa[EC_ALPHABET_SIZE];
    char line[1024];
    ec_alphabet *alpha = arg;

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
            int dist;
            do {
                dist = strtod(p, &p);
            } while (row[k] != r++);
            ec_distmat_set(mat, aa[n], aa[k], dist);
        }
        n += 1;
    }

    return EC_SUCCESS;
}

int
ec_distmat_load_plain(ec_distmat *mat, FILE *stream, void *arg)
{
    size_t i, k;
    ec_dist dist;
    (void) arg;
    for (i = 0; i < EC_ALPHABET_SIZE; i++) {
        for (k = 0; k < EC_ALPHABET_SIZE; k++) {
            if (fscanf(stream, "%" EC_DIST_SCN, &dist) != 1) {
                return EC_FAILURE;
            }
            ec_distmat_set(mat, i, k, dist);
        }
    }
    return 0;
}
