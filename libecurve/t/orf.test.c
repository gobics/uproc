#include <math.h>

#include "test.h"
#include "ecurve.h"

void setup(void)
{
}

void teardown(void)
{
}

int orf(void)
{
    char *buf[EC_ORF_FRAMES] = { NULL };
    size_t sz[EC_ORF_FRAMES];

    DESC("ORF translation");

#define REP(s) s s s s s
#define DNA_F REP("CCNGARACNGARCGN") "TAR" REP("TTYCGNGCNAAYAAR")
#define DNA_R REP("YTTRTTNGCNCGRAA") "YTA" REP("NCGYTCNGTYTCNGG")
#define A REP("PETER")
#define B REP("FRANK")
#define PROT_F A "*" B
#define PROT_R B "*" A

    ec_orf_chained(DNA_F, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_uint_ne(!!buf[0], 0, "");
    assert_str_eq(buf[0], PROT_F, "frame 0 correctly translated");
    ec_orf_chained("A" DNA_F, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_str_eq(buf[1], PROT_F, "frame 1 correctly translated");
    ec_orf_chained("AA" DNA_F, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_str_eq(buf[2], PROT_F, "frame 2 correctly translated");

    ec_orf_chained(DNA_R, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_str_eq(buf[3], PROT_R, "frame 3 correctly translated");
    ec_orf_chained("A" DNA_R, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_str_eq(buf[4], PROT_R, "frame 4 correctly translated");
    ec_orf_chained("AA" DNA_R, EC_ORF_PER_FRAME, NULL, NULL, buf, sz);
    assert_str_eq(buf[5], PROT_R, "frame 5 correctly translated");

    for (size_t i = 0; i < EC_ORF_FRAMES; i++) {
        free(buf[i]);
    }
    return SUCCESS;
}


TESTS_INIT(orf);
