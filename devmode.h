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

#ifndef DEVMODE_H
#define DEVMODE_H

#include <uproc.h>

#if defined(DEVMODE) && HAVE_CLOCK_GETTIME
#define HAVE_TIMEIT 1
#include <time.h>
typedef struct {
    int running;
    double total;
    struct timespec start;
} timeit;

#define TIMEIT_INITIALIZER \
    {                      \
        0, 0,              \
        {                  \
            0, 0           \
        }                  \
    }

void timeit_start(timeit *t);
void timeit_stop(timeit *t);
void timeit_print(timeit *t, const char *s);
#else
#define HAVE_TIMEIT 0
typedef int timeit;
#define TIMEIT_INITIALIZER 0
#define timeit_start(...) ((void)0)
#define timeit_stop(...) ((void)0)
#define timeit_print(...) ((void)0)
#endif

uproc_list *fake_results(void);

#endif
