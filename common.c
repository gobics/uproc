/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if HAVE__MKDIR
#include <direct.h>
#elif HAVE_MKDIR
#include <sys/stat.h>
#endif

#include <uproc.h>

#include "common.h"

#define PROGRESS_WIDTH 20

uproc_io_stream *open_read(const char *path)
{
    if (!path || !strcmp(path, "-")) {
        return uproc_stdin;
    }
    return uproc_io_open("r", UPROC_IO_GZIP, "%s", path);
}

uproc_io_stream *open_write(const char *path, enum uproc_io_type type)
{
    if (!path || !strcmp(path, "-")) {
        return type == UPROC_IO_GZIP ? uproc_stdout_gz : uproc_stdout;
    }
    return uproc_io_open("w", type, "%s", path);
}

void print_version(const char *progname)
{
    printf(
        "%s, version " UPROC_VERSION
        "\n"
        "Copyright 2014 Peter Meinicke, Robin Martinjak\n"
        "License GPLv3+: GNU GPL version 3 or later " /* no line break! */
        "<http://gnu.org/licenses/gpl.html>\n"
        "\n"
        "This is free software; you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Please send bug reports to " PACKAGE_BUGREPORT "\n",
        progname);
}

int parse_int(const char *arg, int *x)
{
    char *end;
    int tmp = strtol(arg, &end, 10);
    if (!*arg || *end) {
        return -1;
    }
    *x = tmp;
    return 0;
}

int parse_db_version(const char *arg, int *x)
{
    int tmp;
    if (parse_int(arg, &tmp)) {
        return -1;
    }
    if (tmp < UPROC_DATABASE_V1 || tmp > UPROC_DATABASE_LATEST) {
        return -1;
    }
    *x = tmp;
    return 0;
}

int parse_prot_thresh_level(const char *arg, int *x)
{
    int tmp;
    if (parse_int(arg, &tmp)) {
        return -1;
    }
    if (tmp != 0 && tmp != 2 && tmp != 3) {
        return -1;
    }
    *x = tmp;
    return 0;
}

int parse_orf_thresh_level(const char *arg, int *x)
{
    int tmp;
    if (parse_int(arg, &tmp)) {
        return -1;
    }
    if (tmp != 0 && tmp != 1 && tmp != 2) {
        return -1;
    }
    *x = tmp;
    return 0;
}

void errhandler_bail(enum uproc_error_code num, const char *msg,
                     const char *loc, void *context)
{
    (void)num;
    (void)msg;
    (void)loc;
    (void)context;
    uproc_perror("");
    exit(EXIT_FAILURE);
}

void trim_header(char *s)
{
    char *start = s, *end;
    while (isspace(*start) || *start == ',') {
        start++;
    }
    end = strpbrk(start, ", \f\n\r\t\v");
    if (end) {
        *end = '\0';
    }
}

void make_dir(const char *path)
{
#if HAVE__MKDIR
    _mkdir(path);
#elif HAVE_MKDIR
    mkdir(path, 0777);
#endif
}

void progress(uproc_io_stream *stream, const char *new_label, double percent)
{
    static const char *label;
    static double last_percent;
    if (new_label) {
        last_percent = -1.0;
        label = new_label;
    }
    if (percent < 0.0 || last_percent >= 100.0 ||
        (!new_label && fabs(percent - last_percent) < 0.05 &&
         percent < 100.0)) {
        return;
    }
    char bar[PROGRESS_WIDTH + 1] = "";
    int p = percent / 100.0 * PROGRESS_WIDTH;
    for (int i = 0; i < PROGRESS_WIDTH; i++) {
        bar[i] = i < p ? '#' : ' ';
    }
    uproc_io_printf(stream, "\r%s: [%s] %5.1f%%", label, bar, percent);
    if (percent >= 100.0) {
        uproc_io_printf(stream, "\n");
    }
    last_percent = percent;
}
