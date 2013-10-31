#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
#include "iupac.h"
#include "upro/orf.h"
#include "upro/codon.h"

#define LOW_BITS(x, n) ((x) & ~(~0 << (n)))
#define NT_AT(c, pos) LOW_BITS(((c) >> (pos) * UPRO_NT_BITS), UPRO_NT_BITS)
#define CODON(nt1, nt2, nt3) ((nt1 << 2 * UPRO_NT_BITS) | (nt2 << UPRO_NT_BITS) | nt3)

#define TABLE(T)                                                        \
void table_ ## T (const char *prefix, const char *fmt, T *v, size_t n)  \
{                                                                       \
    size_t i, width = 0;                                                \
    printf("static " # T " %s[%zu] = {\n", prefix, n);                  \
                                                                        \
    width += printf("   ");                                             \
    for (i = 0; i < n; i++) {                                           \
        if (width > 70) {                                               \
            width = printf("\n    ") - 1;                               \
        }                                                               \
        else {                                                          \
            width += printf(" ");                                       \
        }                                                               \
        width += printf(fmt, v[i]);                                     \
        if (i < n) {                                                    \
            width += printf(",");                                       \
        }                                                               \
    }                                                                   \
    printf("\n};\n");                                                   \
}

TABLE(int)

void codon_append(upro_codon *codon, upro_nt nt)
{
    *codon <<= UPRO_NT_BITS;
    *codon |= nt;
    *codon &= (~(~0ULL << UPRO_CODON_BITS));
}

void gen_char_to_nt(void)
{
    int char_to_nt[UCHAR_MAX + 1];
    size_t i;

    for (i = 0; i < UCHAR_MAX + 1; i++) {
        char_to_nt[i] = iupac_char_to_nt(i);
    }
    table_int("char_to_nt", "%4d", char_to_nt, UCHAR_MAX + 1);
    printf("\n");
}

void gen_codon(void)
{
    int codon_complement[UPRO_BINARY_CODON_COUNT], codon_is_stop[UPRO_BINARY_CODON_COUNT], codon_to_char[UPRO_BINARY_CODON_COUNT];
    size_t i;
    for (i = 0; i < UPRO_BINARY_CODON_COUNT; i++) {
        int k;
        upro_codon c = 0;
        upro_nt nt[3] = { NT_AT(i, 2), NT_AT(i, 1), NT_AT(i, 0) };

        for (k = 3; k--;) {
            upro_nt tmp = 0;
            if (nt[k] & UPRO_NT_A) tmp |= UPRO_NT_T;
            if (nt[k] & UPRO_NT_C) tmp |= UPRO_NT_G;
            if (nt[k] & UPRO_NT_G) tmp |= UPRO_NT_C;
            if (nt[k] & UPRO_NT_T) tmp |= UPRO_NT_A;
            codon_append(&c, tmp);
        }
        codon_complement[i] = c;

        switch (i) {
            case CODON(UPRO_NT_T, UPRO_NT_A, UPRO_NT_A):
            case CODON(UPRO_NT_T, UPRO_NT_A, UPRO_NT_G):
            case CODON(UPRO_NT_T, UPRO_NT_G, UPRO_NT_A):
            /* XXX: R ist G oder A -> auch in stopcodons verwenden? */
            case CODON(UPRO_NT_T, (UPRO_NT_A | UPRO_NT_G), UPRO_NT_A):
            case CODON(UPRO_NT_T, UPRO_NT_A, (UPRO_NT_A | UPRO_NT_G)):
                codon_is_stop[i] = 1;
                break;
            default:
                codon_is_stop[i] = 0;
        }

        #define CASE(nt, aa)                                            \
        else if (upro_codon_match(i, iupac_string_to_codon(nt)))           \
            codon_to_char[i] = aa

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
    table_int("codon_complement", "%5d", codon_complement, UPRO_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_is_stop", "%2d", codon_is_stop, UPRO_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_to_char", "'%c'", codon_to_char, UPRO_BINARY_CODON_COUNT);
}

int main(void)
{
    gen_char_to_nt();
    gen_codon();
    printf("\n"
            "#define CODON_IS_STOP(c) (codon_is_stop[(c)])\n"
            "#define CODON_COMPLEMENT(c) (codon_complement[(c)])\n"
            "#define CODON_TO_CHAR(c) (codon_to_char[(c)])\n"
            "#define CHAR_TO_NT(c) (char_to_nt[(unsigned char) (c)])\n"
          );

    return EXIT_SUCCESS;
}
