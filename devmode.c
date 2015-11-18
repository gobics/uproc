/* Copyright 2014-2015 Peter Meinicke, Robin Martinjak
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "devmode.h"

#include <uproc.h>

#if HAVE_TIMEIT
void timeit_start(timeit *t)
{
    if (t->running) {
        return;
    }
    clock_gettime(CLOCK_REALTIME, &t->start);
    t->running = 1;
}

void timeit_stop(timeit *t)
{
    struct timespec stop;
    if (!t->running) {
        return;
    }
    clock_gettime(CLOCK_REALTIME, &stop);
    t->total += (stop.tv_sec - t->start.tv_sec) +
                (stop.tv_nsec - t->start.tv_nsec) / 1e9;
    t->running = 0;
}

void timeit_print(timeit *t, const char *s)
{
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fprintf(stderr, "%.5f\n", t->total);
}

#endif

static uproc_list *make_mosaicword_list(struct uproc_mosaicword *words, size_t n)
{
    uproc_list *mw = uproc_list_create(sizeof (struct uproc_mosaicword));
    for (int i = 0; i < n; i++) {
        uproc_list_append(mw, &words[i]);
    }
    return mw;
}

uproc_list *fake_results(void)
{
    struct uproc_result fake_results[] = {
        {
            .orf = {
                .data = "QTLDNTKHQMWPHYSLQYRFTK",
                .start = 0,
                .length = 22,
                .score = 13.37,
                .frame = 0,
            },
            .rank = 0,
            .class = 2,
            .score = 3.14,
            .mosaicwords = make_mosaicword_list(
                (struct uproc_mosaicword[]) {
                {
                .word = { 0, 0 },
                .index = 0,
                .score = 3,
                },
                {
                .word = { 61414151, 74614151 },
                .index = 2,
                .score = 0.14,
                },
                }, 2),
        },
        {
            .orf = {
                .data = "LMKWEWPRTYEEICWGSFMSND",
                .start = 1,
                .length = 22,
                .score = 77.34,
                .frame = 5,
            },
            .rank = 0,
            .class = 1,
            .score = 6.28,
            .mosaicwords = make_mosaicword_list(
                (struct uproc_mosaicword[]) {
                {
                .word = { 61414151, 0 },
                .index = 0,
                .score = 77,
                },
                {
                .word = { 1461, 74614151 },
                .index = 2,
                .score = 0.34,
                },
                }, 2),
        },
    };

    uproc_list *results = uproc_list_create(sizeof (struct uproc_result));

    int n = sizeof(fake_results) / sizeof(fake_results[0]);
    for (int i = 0; i < n; i++) {
        struct uproc_result result = fake_results[i];
        uproc_list_append(results, &result);
    }
    return results;
}
