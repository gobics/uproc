/* Translate characters to/from amino acids
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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

int
uproc_alphabet_init(struct uproc_alphabet *alpha, const char *s)
{
    unsigned char i;
    const char *p;

    if (strlen(s) != UPROC_ALPHABET_SIZE) {
        return uproc_error_msg(
            UPROC_EINVAL, "string too short: %d characters instead of %d",
            strlen(s), UPROC_ALPHABET_SIZE);
    }
    strcpy(alpha->str, s);
    for (i = 0; i < UCHAR_MAX; i++) {
        alpha->aminos[i] = -1;
    }
    for (p = s; *p; p++) {
        i = toupper(*p);
        if (!isupper(i)) {
            return uproc_error_msg(UPROC_EINVAL, "invalid character '%c'", i);
        }
        if (alpha->aminos[i] != -1) {
            return uproc_error_msg(UPROC_EINVAL, "duplicate character '%c'", i);
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
uproc_alphabet_amino_to_char(const struct uproc_alphabet *alpha,
                             uproc_amino amino)
{
    if (amino >= UPROC_ALPHABET_SIZE) {
        return -1;
    }
    return alpha->str[amino];
}
