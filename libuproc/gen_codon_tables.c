/* generates codon_tables.h
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
#include "uproc/common.h"
#include "uproc/orf.h"
#include "uproc/codon.h"
#include "codon.c"

#define IUPAC_NT "ACGTU"
#define IUPAC_DNA_WC "RYSWKMBDHVN"
#define IUPAC_DNA_ALL IUPAC_NT IUPAC_DNA_WC

static uproc_nt
iupac_char_to_nt(int c)
{
    uproc_nt nt = 0;
    c = toupper(c);

    if (!isalpha(c)) {
        return UPROC_NT_NOT_CHAR;
    }
    else if (!strchr(IUPAC_DNA_ALL, c)) {
        return UPROC_NT_NOT_IUPAC;
    }

    if (strchr("ARWMDHVN", c))
        nt |= UPROC_NT_A;
    if (strchr("CYSMBHVN", c))
        nt |= UPROC_NT_C;
    if (strchr("GRSKBDVN", c))
        nt |= UPROC_NT_G;
    if (strchr("TUYWKBDHN", c))
        nt |= UPROC_NT_T;
    return nt;
}

static uproc_codon
iupac_string_to_codon(const char *str)
{
    unsigned i;
    uproc_codon c = 0;
    for (i = 0; i < UPROC_CODON_NTS; i++) {
        uproc_codon_append(&c, iupac_char_to_nt(str[i]));
    }
    return c;
}

#define LOW_BITS(x, n) ((x) & ~(~0 << (n)))

#define NT_AT(c, pos) LOW_BITS(((c) >> (pos) * UPROC_NT_BITS), UPROC_NT_BITS)

#define CODON(nt1, nt2, nt3) \
    ((nt1 << 2 * UPROC_NT_BITS) | (nt2 << UPROC_NT_BITS) | nt3)

#define TABLE(T)                                                            \
void table_ ## T (const char *prefix, const char *fmt, T *v, unsigned n)    \
{                                                                           \
    unsigned i, width = 0;                                                  \
    printf("static " # T " %s[%u] = {\n", prefix, n);                       \
                                                                            \
    width += printf("   ");                                                 \
    for (i = 0; i < n; i++) {                                               \
        if (width > 70) {                                                   \
            width = printf("\n    ") - 1;                                   \
        }                                                                   \
        else {                                                              \
            width += printf(" ");                                           \
        }                                                                   \
        width += printf(fmt, v[i]);                                         \
        if (i < n) {                                                        \
            width += printf(",");                                           \
        }                                                                   \
    }                                                                       \
    printf("\n};\n");                                                       \
}

TABLE(int)

void codon_append(uproc_codon *codon, uproc_nt nt)
{
    *codon <<= UPROC_NT_BITS;
    *codon |= nt;
    *codon &= (~(~0ULL << UPROC_CODON_BITS));
}

void gen_char_to_nt(void)
{
    int char_to_nt[UCHAR_MAX + 1];
    unsigned i;

    for (i = 0; i < UCHAR_MAX + 1; i++) {
        char_to_nt[i] = iupac_char_to_nt(i);
    }
    table_int("char_to_nt", "%4d", char_to_nt, UCHAR_MAX + 1);
    printf("\n");
}

void gen_codon(void)
{
    int codon_complement[UPROC_BINARY_CODON_COUNT],
        codon_is_stop[UPROC_BINARY_CODON_COUNT],
        codon_to_char[UPROC_BINARY_CODON_COUNT];
    unsigned i;
    for (i = 0; i < UPROC_BINARY_CODON_COUNT; i++) {
        int k;
        uproc_codon c = 0;
        uproc_nt nt[3] = { NT_AT(i, 2), NT_AT(i, 1), NT_AT(i, 0) };

        for (k = 3; k--;) {
            uproc_nt tmp = 0;
            if (nt[k] & UPROC_NT_A) tmp |= UPROC_NT_T;
            if (nt[k] & UPROC_NT_C) tmp |= UPROC_NT_G;
            if (nt[k] & UPROC_NT_G) tmp |= UPROC_NT_C;
            if (nt[k] & UPROC_NT_T) tmp |= UPROC_NT_A;
            codon_append(&c, tmp);
        }
        codon_complement[i] = c;

        switch (i) {
            case CODON(UPROC_NT_T, UPROC_NT_A, UPROC_NT_A):
            case CODON(UPROC_NT_T, UPROC_NT_A, UPROC_NT_G):
            case CODON(UPROC_NT_T, UPROC_NT_G, UPROC_NT_A):
            /* XXX: R ist G oder A -> auch in stopcodons verwenden? */
            case CODON(UPROC_NT_T, (UPROC_NT_A | UPROC_NT_G), UPROC_NT_A):
            case CODON(UPROC_NT_T, UPROC_NT_A, (UPROC_NT_A | UPROC_NT_G)):
                codon_is_stop[i] = 1;
                break;
            default:
                codon_is_stop[i] = 0;
        }

        #define CASE(nt, aa)                                            \
        else if (uproc_codon_match(i, iupac_string_to_codon(nt)))       \
            codon_to_char[i] = aa                                       \

        if (0) ;
        CASE("GCN", 'A');
        CASE("CGN", 'R');
        CASE("MGR", 'R');
        CASE("AAY", 'N');
        CASE("GAY", 'D');
        CASE("TGY", 'C');
        CASE("CAR", 'Q');
        CASE("GAR", 'E');
        CASE("GGN", 'G');
        CASE("CAY", 'H');
        CASE("ATH", 'I');
        CASE("YTR", 'L');
        CASE("CTN", 'L');
        CASE("AAR", 'K');
        CASE("ATG", 'M');
        CASE("TTY", 'F');
        CASE("CCN", 'P');
        CASE("TCN", 'S');
        CASE("AGY", 'S');
        CASE("ACN", 'T');
        CASE("TGG", 'W');
        CASE("TAY", 'Y');
        CASE("GTN", 'V');
        else codon_to_char[i] = 'X';
    }
    table_int("codon_complement", "%5d", codon_complement,
              UPROC_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_is_stop", "%2d", codon_is_stop, UPROC_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_to_char", "'%c'", codon_to_char, UPROC_BINARY_CODON_COUNT);
}

int main(void)
{
    printf("/* this file was generated by gen_codon_tables.c */\n\n");
    gen_char_to_nt();
    gen_codon();
    printf("\n"
           "#define CODON_IS_STOP(c) (codon_is_stop[(c)])\n"
           "#define CODON_COMPLEMENT(c) ((uproc_codon)codon_complement[(c)])\n"
           "#define CODON_TO_CHAR(c) (codon_to_char[(c)])\n"
           "#define CHAR_TO_NT(c) (char_to_nt[(unsigned char) (c)])\n");

    return EXIT_SUCCESS;
}
