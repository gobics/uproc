#include <stdio.h>
#include "makedb.h"

#define WIDTH 20

void
progress(const char *label, double percent)
{
    unsigned i, p = percent / 100 * WIDTH;
    fprintf(stderr, "\r%s: [", label);
    for (i = 0; i < WIDTH; i++) {
        fputc(i < p ? '#' : ' ', stderr);
    }
    fputs("] ", stderr);
    fprintf(stderr, "%.1f%%", percent);
    if (percent >= 100.0) {
        fputc('\n', stderr);
    }
}
