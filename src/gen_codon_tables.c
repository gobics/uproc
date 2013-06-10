#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
#include "iupac.h"
#include "ecurve/orf.h"
#include "ecurve/codon.h"

#define LOW_BITS(x, n) ((x) & ~(~0 << (n)))
#define NT_AT(c, pos) LOW_BITS(((c) >> (pos) * EC_NT_BITS), EC_NT_BITS)
#define CODON(nt1, nt2, nt3) ((nt1 << 2 * EC_NT_BITS) | (nt2 << EC_NT_BITS) | nt3)

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

void codon_append(ec_codon *codon, ec_nt nt)
{
    *codon <<= EC_NT_BITS;
    *codon |= nt;
    *codon &= (~(~0ULL << EC_CODON_BITS));
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
    int codon_complement[EC_BINARY_CODON_COUNT], codon_is_stop[EC_BINARY_CODON_COUNT], codon_to_char[EC_BINARY_CODON_COUNT];
    size_t i;
    for (i = 0; i < EC_BINARY_CODON_COUNT; i++) {
        int k;
        ec_codon c = 0;
        ec_nt nt[3] = { NT_AT(i, 2), NT_AT(i, 1), NT_AT(i, 0) };

        for (k = 3; k--;) {
            ec_nt tmp = 0;
            if (nt[k] & EC_NT_A) tmp |= EC_NT_T;
            if (nt[k] & EC_NT_C) tmp |= EC_NT_G;
            if (nt[k] & EC_NT_G) tmp |= EC_NT_C;
            if (nt[k] & EC_NT_T) tmp |= EC_NT_A;
            codon_append(&c, tmp);
        }
        codon_complement[i] = c;

        switch (i) {
            case CODON(EC_NT_T, EC_NT_A, EC_NT_A):
            case CODON(EC_NT_T, EC_NT_A, EC_NT_G):
            case CODON(EC_NT_T, EC_NT_G, EC_NT_A):
            /* XXX: R ist G oder A -> auch in stopcodons verwenden? */
            case CODON(EC_NT_T, (EC_NT_A | EC_NT_G), EC_NT_A):
            case CODON(EC_NT_T, EC_NT_A, (EC_NT_A | EC_NT_G)):
                codon_is_stop[i] = 1;
                break;
            default:
                codon_is_stop[i] = 0;
        }

        #define CASE(nt, aa)                                            \
        else if (ec_codon_match(i, iupac_string_to_codon(nt)))           \
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
    table_int("codon_complement", "%5d", codon_complement, EC_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_is_stop", "%2d", codon_is_stop, EC_BINARY_CODON_COUNT);
    printf("\n");
    table_int("codon_to_char", "'%c'", codon_to_char, EC_BINARY_CODON_COUNT);
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
