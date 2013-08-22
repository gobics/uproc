#ifndef TIMEIT_H
#define TIMEIT_H
#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <time.h>

struct timeit {
    struct timespec start;
    double total;
} TIMEIT_INITIALIZER;

void
timeit_start(struct timeit *t)
{
    clock_gettime(CLOCK_REALTIME, &t->start);
}

void
timeit_stop(struct timeit *t)
{
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    t->total += (end.tv_sec - t->start.tv_sec) + (end.tv_nsec - t->start.tv_nsec) / 1e9;
}

#endif
