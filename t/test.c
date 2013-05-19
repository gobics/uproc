#include "test.h"

#include <stdio.h>
#include <stdlib.h>

char test_desc[1024], test_error[1024], test_info[1024];

#define FMT_BASE "%s %d - %s"
#define FMT FMT_BASE "\n"
#define FMT_SKIP FMT_BASE " # SKIP: %s\n"
int main(void)
{
    int i, res;
    int n_tests, n_failures = 0;

    setup();

    for (n_tests = 0; tests[n_tests]; n_tests++);

    fprintf(stdout, "1..%d\n", n_tests);

    for (i = 0; i < n_tests; i++) {
        res = tests[i]();
        if (res == SUCCESS) {
            fprintf(stdout, FMT, "ok", i + 1, test_desc);
        }
        else if (res == SKIP) {
            fprintf(stdout, FMT_SKIP, "ok", i + 1, test_desc, test_info);
        }
        else {
            fprintf(stdout, FMT, "not ok", i + 1, test_desc);
            fprintf(stdout, TAPDIAG "%s", test_error);
            if (test_info[0]) {
                fprintf(stdout, TAPDIAG "%s\n", test_info);
            }
            n_failures++;
        }
    }

    teardown();
    return (n_failures > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
