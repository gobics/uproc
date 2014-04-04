/* Translate characters to/from amino acids
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/alphabet.h"

/** Struct defining an amino acid alphabet */
struct uproc_alphabet_s
{
    /** Original alphabet string */
    char str[UPROC_ALPHABET_SIZE + 1];

    /** Lookup table for mapping characters to amino acids */
    uproc_amino aminos[UCHAR_MAX + 1];
};

uproc_alphabet *
uproc_alphabet_create(const char *s)
{
    unsigned char i;
    const char *p;
    struct uproc_alphabet_s *a = malloc(sizeof *a);
    if (!a) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    if (strlen(s) != UPROC_ALPHABET_SIZE) {
        uproc_error_msg(
            UPROC_EINVAL, "string too short: %lu characters instead of %d",
            (unsigned long)strlen(s), UPROC_ALPHABET_SIZE);
        goto error;
    }
    strcpy(a->str, s);
    for (i = 0; i < UCHAR_MAX; i++) {
        a->aminos[i] = -1;
    }
    for (p = s; *p; p++) {
        i = toupper(*p);
        if (!isupper(i)) {
            uproc_error_msg(UPROC_EINVAL, "invalid character '%c'", i);
            goto error;
        }
        if (a->aminos[i] != -1) {
            uproc_error_msg(UPROC_EINVAL, "duplicate character '%c'", i);
            goto error;
        }
        a->aminos[i] = p - s;
    }
    return a;
error:
    free(a);
    return NULL;
}

void
uproc_alphabet_destroy(uproc_alphabet *alpha)
{
    free(alpha);
}

uproc_amino
uproc_alphabet_char_to_amino(const uproc_alphabet *alpha, int c)
{
    return alpha->aminos[(unsigned char)c];
}

int
uproc_alphabet_amino_to_char(const uproc_alphabet *alpha,
                             uproc_amino amino)
{
    if (amino >= UPROC_ALPHABET_SIZE) {
        return -1;
    }
    return alpha->str[amino];
}

const char *
uproc_alphabet_str(const uproc_alphabet *alpha)
{
    return alpha->str;
}
