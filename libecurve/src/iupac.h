#ifndef EC_IUPAC_H
#define EC_IUPAC_H

#include "ecurve/codon.h"
#include "ecurve/common.h"

#define IUPAC_NT "ACGTU"
#define IUPAC_DNA_WC "RYSWKMBDHVN"
#define IUPAC_DNA_ALL IUPAC_NT IUPAC_DNA_WC

static ec_nt
iupac_char_to_nt(int c)
{
    ec_nt nt = 0;
    c = toupper(c);

    if (!isalpha(c)) {
        return EC_NT_NOT_CHAR;
    }
    else if (!strchr(IUPAC_DNA_ALL, c)) {
        return EC_NT_NOT_IUPAC;
    }

    if (strchr("ARWMDHVN", c))
        nt |= EC_NT_A;
    if (strchr("CYSMBHVN", c))
        nt |= EC_NT_C;
    if (strchr("GRSKBDVN", c))
        nt |= EC_NT_G;
    if (strchr("TUYWKBDHN", c))
        nt |= EC_NT_T;
    return nt;
}

static ec_codon
iupac_string_to_codon(const char *str)
{
    unsigned i;
    ec_codon c = 0;
    for (i = 0; i < EC_CODON_NTS; i++) {
        ec_codon_append(&c, iupac_char_to_nt(str[i]));
    }
    return c;
}

#endif
