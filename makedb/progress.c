/* uproc-makedb
 * Create a new uproc database.
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
 *
 * This file is part of uproc.
 *
 * uproc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * uproc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "makedb.h"

#define WIDTH 20

void
progress(const char *new_label, double percent)
{
    static const char *label;
    unsigned i, p = percent / 100 * WIDTH;
    if (new_label) {
        label = new_label;
    }
    if (percent < 0.0) {
        return;
    }
    fprintf(stderr, "\r%s: [", label);
    for (i = 0; i < WIDTH; i++) {
        fputc(i < p ? '#' : ' ', stderr);
    }
    fputs("] ", stderr);
    fprintf(stderr, "%.1f%%", percent);
    if (percent >= 100.0) {
        fputc('\n', stderr);
    }
}
