#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "ecurve/common.h"
#include "ecurve/codon.h"

ec_nt
ec_codon_get_nt(ec_codon codon, unsigned position)
{
    if (position >= EC_CODON_NTS) {
        return 0;
    }
    return codon >> position * EC_NT_BITS & EC_BITMASK(EC_NT_BITS);
}

bool
ec_codon_match(ec_codon codon, ec_codon mask)
{
    unsigned i;
    ec_nt c, m;
    for (i = 0; i < EC_CODON_NTS; i++) {
        c = ec_codon_get_nt(codon, i);
        m = ec_codon_get_nt(mask, i);
        if (!c || (c & m) != c) {
            return false;
        }
    }
    return true;
}

void
ec_codon_append(ec_codon *codon, ec_nt nt)
{
    *codon <<= EC_NT_BITS;
    *codon |= nt;
    *codon &= EC_BITMASK(EC_CODON_BITS);
}

void
ec_codon_prepend(ec_codon *codon, ec_nt nt)
{
    *codon >>= EC_NT_BITS;
    *codon |= nt << ((EC_CODON_NTS - 1) * EC_NT_BITS);
}
