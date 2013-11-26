/* uproc-import and uproc-export
 * Convert database from portable to 'native' format and vice-versa.
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_GETOPT_LONG
#define _GNU_SOURCE
#include <getopt.h>
#define OPT(shortopt, longopt) shortopt ", " longopt "    "
#else
#define OPT(shortopt, longopt) shortopt "    "
#endif
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "uproc.h"

#ifdef EXPORT
#define IN_FORMAT UPROC_STORAGE_BINARY
#define IN_EXT ".bin"
#define OUT_FORMAT UPROC_STORAGE_PLAIN
#define OUT_EXT ".plain"
#else
#define IN_FORMAT UPROC_STORAGE_PLAIN
#define IN_EXT ".plain"
#define OUT_FORMAT UPROC_STORAGE_BINARY
#define OUT_EXT ".bin"
#endif

void
print_version(void)
{
    fputs(
        PROGNAME ", version " PACKAGE_VERSION "\n"
        "Copyright 2013 Peter Meinicke, Robin Martinjak\n"
        "License GPLv3+: GNU GPL version 3 or later " /* no line break! */
        "<http://gnu.org/licenses/gpl.html>\n"
        "\n"
        "This is free software; you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Please send bug reports to " PACKAGE_BUGREPORT "\n",
        stderr);
}

void
print_usage(const char *progname)
{
    fprintf(
        stderr,
        PROGNAME ", version " PACKAGE_VERSION "\n"
        "\n"
        "USAGE: %s [options] SOURCEDIR DESTDIR\n"
#ifdef EXPORT
        "Export "
#else
        "Import "
#endif
        "uproc database from SOURCEDIR to DESTDIR (which must exist).\n"
        "\n"
        "GENERAL OPTIONS:\n"
        OPT("-h", "--help      ") "Print this message and exit.\n"
        OPT("-v", "--version   ") "Print version and exit.\n"
        OPT("-n", "--nocompress") "Don't store using gzip compression\n",
        progname);
}


int
convert_ecurve(const char *srcdir, const char *destdir, const char *name,
               enum uproc_io_type iotype)
{
    int res;
    struct uproc_ecurve ec;
    res = uproc_storage_load(&ec, IN_FORMAT, iotype,
                             "%s/%s.%s", srcdir, name, IN_EXT);
    if (res) {
        uproc_perror("error loading %s/%s.%s", srcdir, name, IN_EXT);
        return res;
    }
    res = uproc_storage_store(&ec, OUT_FORMAT, iotype,
                             "%s/%s.%s", destdir, name, OUT_EXT);
    uproc_ecurve_destroy(&ec);
    if (res) {
        uproc_perror("error storing %s/%s.%s", destdir, name, OUT_EXT);
    }
    return res;
}

int
convert_matrix(const char *srcdir, const char *destdir, const char *name)
{
    int res;
    struct uproc_matrix m;
    res = uproc_matrix_load(&m, UPROC_IO_GZIP, "%s/%s", srcdir, name);
    if (res) {
        uproc_perror("error loading %s/%s", srcdir, name);
        return res;
    }
    res = uproc_matrix_store(&m, UPROC_IO_STDIO, "%s/%s", destdir, name);
    uproc_matrix_destroy(&m);
    if (res) {
        uproc_perror("error storing %s/%s", destdir, name);
    }
    return res;
}

enum args
{
    SOURCEDIR = 1,
    DESTDIR,
    ARGC
};

int main(int argc, char **argv)
{
    int res, opt;
    enum uproc_io_type iotype = UPROC_IO_GZIP;

#define SHORT_OPTS "hvn"
#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",       no_argument,    NULL, 'h' },
        { "version",    no_argument,    NULL, 'v' },
        { "nocompress", no_argument,    NULL, 'n' },
        { 0, 0, 0, 0 }
    };
    while ((opt = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL)) != -1)
#else
    while ((opt = getopt(argc, argv, SHORT_OPTS)) != -1)
#endif
    {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                print_version();
                return EXIT_SUCCESS;
            case 'n':
                iotype = UPROC_IO_STDIO;
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    const char *src = argv[optind + SOURCEDIR], *dst = argv[optind + DESTDIR];
    res = convert_ecurve(src, dst, "fwd", iotype);
    if (res) {
        return EXIT_FAILURE;
    }
    res = convert_ecurve(src, dst, "rev", iotype);
    if (res) {
        return EXIT_FAILURE;
    }

    res = convert_matrix(src, dst, "prot_thresh_e2");
    if (res) {
        return EXIT_FAILURE;
    }
    res = convert_matrix(src, dst, "prot_thresh_e3");
    if (res) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
