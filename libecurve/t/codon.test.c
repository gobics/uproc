#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "test.h"
#include "ecurve.h"
#include "../src/iupac.h"

#define CODON(s) iupac_string_to_codon(s)

void setup(void)
{}
void teardown(void)
{}

int append(void)
{
    ec_codon c = CODON("AAA");

    DESC("appending nt to codon");

    ec_codon_append(&c, EC_NT_C);
    assert_uint_eq(c, CODON("AAC"), "codons equal");

    ec_codon_append(&c, EC_NT_T);
    assert_uint_eq(c, CODON("ACT"), "codons equal");

    ec_codon_append(&c, EC_NT_G);
    assert_uint_eq(c, CODON("CTG"), "codons equal");

    return SUCCESS;
}

int prepend(void)
{
    ec_codon c = CODON("AAA");

    DESC("prepending nt to codon");

    ec_codon_prepend(&c, EC_NT_C);
    assert_uint_eq(c, CODON("CAA"), "codons equal");

    ec_codon_prepend(&c, EC_NT_T);
    assert_uint_eq(c, CODON("TCA"), "codons equal");

    ec_codon_prepend(&c, EC_NT_G);
    assert_uint_eq(c, CODON("GTC"), "codons equal");

    return SUCCESS;
}

int match(void)
{
    DESC("matching codons");

#define M(a, b, res, msg) \
    assert_uint_eq(ec_codon_match(CODON(a), CODON(b)), res, a " " msg " " b)

#define MATCH(a, b) M(a, b, true, "matches")
#define NOMATCH(a, b) M(a, b, false, "doesn't match")

    MATCH("AAA", "AAN");
    MATCH("AGA", "ANA");
    MATCH("AGA", "ANN");
    MATCH("AGA", "NNN");
    MATCH("AGA", "ADA");
    MATCH("TTT", "WKY");
    MATCH("TTT", "BDH");

    NOMATCH("AAA", "ASA");
    NOMATCH("TAC", "VAC");
    NOMATCH("GCT", "HBV");
    NOMATCH("AAT", "NNA");
    NOMATCH("TGA", "ATA");

    return SUCCESS;
}

TESTS_INIT(append, prepend, match);
