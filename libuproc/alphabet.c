#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/alphabet.h"

int
uproc_alphabet_init(struct uproc_alphabet *alpha, const char *s)
{
    unsigned char i;
    const char *p;

    if (strlen(s) != UPROC_ALPHABET_SIZE) {
        return uproc_error_msg(UPROC_EINVAL,
                              "input string too short: %d chars instead of %d",
                              strlen(s), UPROC_ALPHABET_SIZE);
    }
    strcpy(alpha->str, s);
    for (i = 0; i < UCHAR_MAX; i++) {
        alpha->aminos[i] = -1;
    }
    for (p = s; *p; p++) {
        i = *p;
        /* invalid or duplicate character */
        if (!isupper(i)) {
            return uproc_error_msg(UPROC_EINVAL,
                                  "string contains non-uppercase character");
        }
        if (alpha->aminos[i] != -1) {
            return uproc_error_msg(UPROC_EINVAL,
                                  "duplicate '%c' in input string", i);
        }
        alpha->aminos[i] = p - s;
    }
    return 0;
}

uproc_amino
uproc_alphabet_char_to_amino(const struct uproc_alphabet *alpha, int c)
{
    return alpha->aminos[(unsigned char)c];
}

int
uproc_alphabet_amino_to_char(const struct uproc_alphabet *alpha, uproc_amino amino)
{
    if (amino >= UPROC_ALPHABET_SIZE) {
        return -1;
    }
    return alpha->str[amino];
}
