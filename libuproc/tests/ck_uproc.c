#include <stdlib.h>
#include <check.h>
#include "uproc.h"

Suite *common_suite(void);
Suite *alphabet_suite(void);

int main(void)
{
    int n_failed;
    SRunner *sr = srunner_create(common_suite());
    srunner_add_suite(sr, alphabet_suite());
    srunner_add_suite(sr, idmap_suite());
    srunner_run_all(sr, CK_NORMAL);
    n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
