/* Operations on tri-nucleotide codons
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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "uproc/common.h"
#include "uproc/codon.h"

uproc_nt
uproc_codon_get_nt(uproc_codon codon, unsigned position)
{
    if (position >= UPROC_CODON_NTS) {
        return 0;
    }
    return codon >> position * UPROC_NT_BITS & UPROC_BITMASK(UPROC_NT_BITS);
}

bool
uproc_codon_match(uproc_codon codon, uproc_codon mask)
{
    unsigned i;
    uproc_nt c, m;
    for (i = 0; i < UPROC_CODON_NTS; i++) {
        c = uproc_codon_get_nt(codon, i);
        m = uproc_codon_get_nt(mask, i);
        if (!c || (c & m) != c) {
            return false;
        }
    }
    return true;
}

void
uproc_codon_append(uproc_codon *codon, uproc_nt nt)
{
    *codon <<= UPROC_NT_BITS;
    *codon |= nt;
    *codon &= UPROC_BITMASK(UPROC_CODON_BITS);
}

void
uproc_codon_prepend(uproc_codon *codon, uproc_nt nt)
{
    *codon >>= UPROC_NT_BITS;
    *codon |= nt << ((UPROC_CODON_NTS - 1) * UPROC_NT_BITS);
}
