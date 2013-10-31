#ifndef UPRO_IUPAC_H
#define UPRO_IUPAC_H

#include "upro/codon.h"
#include "upro/common.h"

#define IUPAC_NT "ACGTU"
#define IUPAC_DNA_WC "RYSWKMBDHVN"
#define IUPAC_DNA_ALL IUPAC_NT IUPAC_DNA_WC

static upro_nt
iupac_char_to_nt(int c)
{
    upro_nt nt = 0;
    c = toupper(c);

    if (!isalpha(c)) {
        return UPRO_NT_NOT_CHAR;
    }
    else if (!strchr(IUPAC_DNA_ALL, c)) {
        return UPRO_NT_NOT_IUPAC;
    }

    if (strchr("ARWMDHVN", c))
        nt |= UPRO_NT_A;
    if (strchr("CYSMBHVN", c))
        nt |= UPRO_NT_C;
    if (strchr("GRSKBDVN", c))
        nt |= UPRO_NT_G;
    if (strchr("TUYWKBDHN", c))
        nt |= UPRO_NT_T;
    return nt;
}

static upro_codon
iupac_string_to_codon(const char *str)
{
    unsigned i;
    upro_codon c = 0;
    for (i = 0; i < UPRO_CODON_NTS; i++) {
        upro_codon_append(&c, iupac_char_to_nt(str[i]));
    }
    return c;
}

#endif
