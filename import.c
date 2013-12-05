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
#include "uproc_opt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "uproc.h"

void
print_usage(const char *progname)
{
    fprintf(
        stderr,
        PROGNAME ", version " PACKAGE_VERSION "\n"
        "\n"
#ifdef EXPORT
        "USAGE: %s [options] SOURCEDIR DEST\n"
        "Export database from SOURCEDIR to DEST.\n"
#else
        "USAGE: %s [options] SOURCE DESTDIR\n"
        "Import database from SOURCE to DESTDIR (which must exist).\n"
#endif
        "\n"
        "GENERAL OPTIONS:\n"
        OPT("-h", "--help      ", "") "Print this message and exit.\n"
        OPT("-v", "--version   ", "") "Print version and exit.\n"
#ifdef EXPORT
        OPT("-n", "--nocompress", "") "Don't store using gzip compression\n"
#endif
        ,
        progname);
}

int
export_ecurve(const char *dir, const char *name, uproc_io_stream *stream)
{
    int res;
    struct uproc_ecurve ec;
    fprintf(stderr, "exporting %s/%s.ecurve\n", dir, name);
    res = uproc_storage_load(&ec, UPROC_STORAGE_BINARY, UPROC_IO_GZIP,
                             "%s/%s.ecurve", dir, name);
    if (res) {
        return res;
    }
    res = uproc_storage_stores(&ec, UPROC_STORAGE_PLAIN, stream);
    uproc_ecurve_destroy(&ec);
    return res;
}

int
import_ecurve(const char *dir, const char *name, uproc_io_stream *stream)
{
    int res;
    struct uproc_ecurve ec;
    fprintf(stderr, "importing %s/%s.ecurve (this will take several minutes)\n", dir, name);
    res = uproc_storage_loads(&ec, UPROC_STORAGE_PLAIN, stream);
    if (res) {
        return res;
    }
    res = uproc_storage_store(&ec, UPROC_STORAGE_BINARY, UPROC_IO_GZIP,
                              "%s/%s.ecurve", dir, name);
    uproc_ecurve_destroy(&ec);
    return res;
}

int
export_matrix(const char *dir, const char *name, uproc_io_stream *stream)
{
    int res;
    struct uproc_matrix m;
    fprintf(stderr, "exporting %s/%s\n", dir, name);
    res = uproc_matrix_load(&m, UPROC_IO_GZIP, "%s/%s", dir, name);
    if (res) {
        return res;
    }
    res = uproc_matrix_stores(&m, stream);
    uproc_matrix_destroy(&m);
    return res;
}

int
import_matrix(const char *dir, const char *name, uproc_io_stream *stream)
{
    int res;
    struct uproc_matrix m;
    fprintf(stderr, "importing %s/%s\n", dir, name);
    res = uproc_matrix_loads(&m, stream);
    if (res) {
        return res;
    }
    res = uproc_matrix_store(&m, UPROC_IO_GZIP, "%s/%s", dir, name);
    uproc_matrix_destroy(&m);
    return res;
}

enum args
{
    SOURCE,
    DEST,
    ARGC
};

int main(int argc, char **argv)
{
    int res, opt;
    enum uproc_io_type iotype = UPROC_IO_GZIP;

#ifdef EXPORT
#define SHORT_OPTS "hvn"
#else
#define SHORT_OPTS "hv"
#endif

#if HAVE_GETOPT_LONG
    struct option long_opts[] = {
        { "help",       no_argument,    NULL, 'h' },
        { "version",    no_argument,    NULL, 'v' },
#ifdef EXPORT
        { "nocompress", no_argument,    NULL, 'n' },
#endif
        { 0, 0, 0, 0 }
    };
#else
#define long_opts NULL
#endif
    while ((opt = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL)) != -1)
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
    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    uproc_io_stream *stream;
    const char *src = argv[optind + SOURCE], *dest = argv[optind + DEST];

#ifdef EXPORT
    stream = uproc_io_open("w", iotype, "%s", dest);
    if (!stream) {
        uproc_perror("error opening %s", dest);
        return EXIT_FAILURE;
    }
    res = export_ecurve(src, "fwd", stream);
    if (res) {
        uproc_perror("error exporting forward ecurve");
        return EXIT_FAILURE;
    }
    res = export_ecurve(src, "rev", stream);
    if (res) {
        uproc_perror("error exporting reverse ecurve");
        return EXIT_FAILURE;
    }
    res = export_matrix(src, "prot_thresh_e2", stream);
    if (res) {
        uproc_perror("error exporting matrix");
        return EXIT_FAILURE;
    }
    res = export_matrix(src, "prot_thresh_e3", stream);
    if (res) {
        uproc_perror("error exporting matrix");
        return EXIT_FAILURE;
    }
    uproc_io_close(stream);
    return EXIT_SUCCESS;
#else
    stream = uproc_io_open("r", UPROC_IO_GZIP, "%s", src);
    if (!stream) {
        uproc_perror("error opening %s", src);
        return EXIT_FAILURE;
    }
    res = import_ecurve(dest, "fwd", stream);
    if (res) {
        uproc_perror("error importing forward ecurve");
        return EXIT_FAILURE;
    }
    res = import_ecurve(dest, "rev", stream);
    if (res) {
        uproc_perror("error importing reverse ecurve");
        return EXIT_FAILURE;
    }
    res = import_matrix(dest, "prot_thresh_e2", stream);
    if (res) {
        uproc_perror("error importing matrix");
        return EXIT_FAILURE;
    }
    res = import_matrix(dest, "prot_thresh_e3", stream);
    if (res) {
        uproc_perror("error importing matrix");
        return EXIT_FAILURE;
    }
    uproc_io_close(stream);
    return EXIT_SUCCESS;
#endif
}
