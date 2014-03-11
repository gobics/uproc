/* uproc-import and uproc-export
 * Convert database from portable to 'native' format and vice-versa.
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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

#ifdef EXPORT
#define EXPORT_FUNC(name, type, load, store, destroy)                       \
int                                                                         \
name(const char *dir, const char *name, uproc_io_stream *stream)            \
{                                                                           \
    int res;                                                                \
    type *thing;                                                            \
    fprintf(stderr, "exporting %s/%s\n", dir, name);                        \
    thing = load(UPROC_IO_GZIP, "%s/%s", dir, name);                        \
    if (!thing) {                                                           \
        return -1;                                                          \
    }                                                                       \
    res = store(thing, stream);                                             \
    destroy(thing);                                                         \
    return res;                                                             \
}

#define ECURVE_LOAD(iotype, fmt, dir, name) \
    uproc_ecurve_load(UPROC_ECURVE_BINARY, iotype, fmt, dir, name)
#define ECURVE_STORES(ptr, stream) \
    uproc_ecurve_stores(ptr, UPROC_ECURVE_PLAIN, stream)
EXPORT_FUNC(ecurve, uproc_ecurve, ECURVE_LOAD, ECURVE_STORES,
            uproc_ecurve_destroy)
EXPORT_FUNC(matrix, uproc_matrix, uproc_matrix_load, uproc_matrix_stores,
            uproc_matrix_destroy)
EXPORT_FUNC(idmap, uproc_idmap, uproc_idmap_load, uproc_idmap_stores,
            uproc_idmap_destroy)
#else
#define IMPORT_FUNC(name, type, load, store, destroy)                       \
int                                                                         \
name(const char *dir, const char *name, uproc_io_stream *stream)            \
{                                                                           \
    int res;                                                                \
    type *thing;                                                            \
    fprintf(stderr, "importing %s/%s\n", dir, name);                        \
    thing = load(stream);                                                   \
    if (!thing) {                                                           \
        return -1;                                                          \
    }                                                                       \
    res = store(thing, UPROC_IO_GZIP, "%s/%s", dir, name);                  \
    destroy(thing);                                                         \
    return res;                                                             \
}

#define ECURVE_LOADS(stream) \
    uproc_ecurve_loads(UPROC_ECURVE_PLAIN, stream)
#define ECURVE_STORE(ptr, iotype, fmt, dir, name) \
    uproc_ecurve_store(ptr, UPROC_ECURVE_BINARY, iotype, fmt, dir, name)
IMPORT_FUNC(ecurve, uproc_ecurve, ECURVE_LOADS, ECURVE_STORE,
            uproc_ecurve_destroy)
IMPORT_FUNC(matrix, uproc_matrix, uproc_matrix_loads, uproc_matrix_store,
            uproc_matrix_destroy)
IMPORT_FUNC(idmap, uproc_idmap, uproc_idmap_loads, uproc_idmap_store,
            uproc_idmap_destroy)
#endif

enum args
{
    SOURCE,
    DEST,
    ARGC
};

int main(int argc, char **argv)
{
    int res, opt;
    uproc_io_stream *stream;
    const char *dir, *file;

#ifdef EXPORT
    enum uproc_io_type iotype = UPROC_IO_GZIP;
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
#ifdef EXPORT
            case 'n':
                iotype = UPROC_IO_STDIO;
                break;
#endif
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
#ifdef EXPORT
#define IMEX "ex"
    dir = argv[optind + SOURCE];
    file = argv[optind + DEST];
    stream = uproc_io_open("w", iotype, "%s", file);
#else
#define IMEX "im"
    dir = argv[optind + DEST];
    file = argv[optind + SOURCE];
    stream = uproc_io_open("r", UPROC_IO_GZIP, "%s", file);
#endif
    if (!stream) {
        uproc_perror("error opening %s", file);
        return EXIT_FAILURE;
    }
    res = matrix(dir, "prot_thresh_e2", stream);
    if (res) {
        uproc_perror("error " IMEX"porting matrix");
        return EXIT_FAILURE;
    }
    res = matrix(dir, "prot_thresh_e3", stream);
    if (res) {
        uproc_perror("error " IMEX"porting matrix");
        return EXIT_FAILURE;
    }
    res = idmap(dir, "idmap", stream);
    if (res) {
        uproc_perror("error " IMEX"porting idmap");
        return EXIT_FAILURE;
    }
    res = ecurve(dir, "fwd.ecurve", stream);
    if (res) {
        uproc_perror("error " IMEX"porting forward ecurve");
        return EXIT_FAILURE;
    }
    res = ecurve(dir, "rev.ecurve", stream);
    if (res) {
        uproc_perror("error " IMEX"porting reverse ecurve");
        return EXIT_FAILURE;
    }
    uproc_io_close(stream);
    return EXIT_SUCCESS;
}
