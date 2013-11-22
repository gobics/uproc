#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "upro/common.h"
#include "upro/codon.h"

upro_nt
upro_codon_get_nt(upro_codon codon, unsigned position)
{
    if (position >= UPRO_CODON_NTS) {
        return 0;
    }
    return codon >> position * UPRO_NT_BITS & UPRO_BITMASK(UPRO_NT_BITS);
}

bool
upro_codon_match(upro_codon codon, upro_codon mask)
{
    unsigned i;
    upro_nt c, m;
    for (i = 0; i < UPRO_CODON_NTS; i++) {
        c = upro_codon_get_nt(codon, i);
        m = upro_codon_get_nt(mask, i);
        if (!c || (c & m) != c) {
            return false;
        }
    }
    return true;
}

void
upro_codon_append(upro_codon *codon, upro_nt nt)
{
    *codon <<= UPRO_NT_BITS;
    *codon |= nt;
    *codon &= UPRO_BITMASK(UPRO_CODON_BITS);
}

void
upro_codon_prepend(upro_codon *codon, upro_nt nt)
{
    *codon >>= UPRO_NT_BITS;
    *codon |= nt << ((UPRO_CODON_NTS - 1) * UPRO_NT_BITS);
}
