#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "ecurve/common.h"
#include "ecurve/alphabet.h"

int
ec_alphabet_init(struct ec_alphabet *alpha, const char *s)
{
    unsigned char i;
    const char *p;

    if (strlen(s) != EC_ALPHABET_SIZE) {
        return EC_FAILURE;
    }
    strcpy(alpha->str, s);
    for (i = 0; i < UCHAR_MAX; i++) {
        alpha->amino_table[i] = -1;
    }
    for (p = s; *p; p++) {
        i = *p;
        /* invalid or duplicate character */
        if (!isupper(i) || alpha->amino_table[i] != -1) {
            return EC_FAILURE;
        }
        alpha->amino_table[i] = p - s;
    }
    return EC_SUCCESS;
}

ec_amino
ec_alphabet_char_to_amino(const struct ec_alphabet *alpha, int c)
{
    return alpha->amino_table[(unsigned char)c];
}

int
ec_alphabet_amino_to_char(const struct ec_alphabet *alpha, ec_amino amino)
{
    if (amino >= EC_ALPHABET_SIZE) {
        return -1;
    }
    return alpha->str[amino];
}
