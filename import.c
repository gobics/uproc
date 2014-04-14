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
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <uproc.h>

#include "ppopts.h"

#ifdef EXPORT
#define PROGNAME "uproc-export"
#else
#define PROGNAME "uproc-import"
#endif


void
make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
#ifdef EXPORT
    ppopts_add_text(o, "USAGE: %s [options] SRCDIR DEST", progname);
    ppopts_add_text(o, "Export database from SRCDIR to DEST.");
#else
    ppopts_add_text(o, "USAGE: %s [options] SRC DESTDIR", progname);
    ppopts_add_text(o, "Import database from SRC to DESTDIR"
#if !defined(HAVE_MKDIR) && !defined(HAVE__MKDIR)
        " (which must already exist)"
#endif
        ".");
#endif

    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help",       "", "Print this message and exit.");
    O('v', "version",    "", "Print version and exit.");
    O('V', "libversion", "", "Print libuproc version/features and exit.");
#ifdef EXPORT
    if (uproc_features_zlib()) {
        O('n', "nocompress", "", "Store without gzip compression.");
    }
#endif
#undef O
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


int
db_info(const char *dir, uproc_io_stream *stream)
{
    char *line = NULL;
    size_t sz;
    uproc_io_stream *in, *out;
#ifdef EXPORT
    fprintf(stderr, "exporting %s/info.txt\n", dir);
    in = uproc_io_open("r", UPROC_IO_GZIP, "%s/info.txt", dir);
    /* info.txt is not crucial, so ignore this error */
    if (!in) {
        return 0;
    }
    out = stream;
#else
    fprintf(stderr, "importing %s/info.txt\n", dir);
    in = stream;
    out = uproc_io_open("w", UPROC_IO_STDIO, "%s/info.txt", dir);
    if (!out) {
        return -1;
    }
#endif
    while (uproc_io_getline(&line, &sz, in) != -1) {
        uproc_io_printf(out, "%s", line);
    }
    free(line);
    return 0;
}


enum nonopt_args
{
    SOURCE,
    DEST,
    ARGC
};

int main(int argc, char **argv)
{
    int res;
    uproc_io_stream *stream;
    const char *dir, *file;

#ifdef EXPORT
    enum uproc_io_type iotype = UPROC_IO_GZIP;
#endif

    int opt;
    struct ppopts opts = PPOPTS_INITIALIZER;
    make_opts(&opts, argv[0]);
    while ((opt = ppopts_getopt(&opts, argc, argv)) != -1) {
        switch (opt) {
            case 'h':
                ppopts_print(&opts, stderr, 80, 0);
                return EXIT_SUCCESS;
            case 'v':
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'V':
                uproc_features_print(uproc_stderr);
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
        ppopts_print(&opts, stderr, 80, 0);
        return EXIT_FAILURE;
    }
#ifdef EXPORT
#define IMEX "ex"
    dir = argv[optind + SOURCE];
    file = argv[optind + DEST];
    stream = open_write(file, iotype);
#else
#define IMEX "im"
    dir = argv[optind + DEST];
    make_dir(dir);
    file = argv[optind + SOURCE];
    stream = open_read(file);
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
    res = db_info(dir, stream);
    if (res) {
        uproc_perror("error " IMEX"porting info.txt");
        return EXIT_FAILURE;
    }
    uproc_io_close(stream);
    return EXIT_SUCCESS;
}
