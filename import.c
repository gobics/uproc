/* uproc-import
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

#define PROGNAME "uproc-import"

static int load_version_ = UPROC_DATABASE_LATEST;
static int store_version_ = UPROC_DATABASE_LATEST;

#define S(x) #x
#define DEFAULT_VERSION S(UPROC_DATABASE_V2)

void make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
    ppopts_add_text(o, "USAGE: %s [options] SRC DESTDIR", progname);
    ppopts_add_text(o,
                    "Import database from SRC to DESTDIR"
#if !defined(HAVE_MKDIR) && !defined(HAVE__MKDIR)
                    " (which must already exist)"
#endif
                    ".");

    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help", "", "Print this message and exit.");
    O('v', "version", "", "Print version and exit.");
    O('V', "libversion", "", "Print libuproc version/features and exit.");

    O('L', "load-version", "VERSION",
      "Use database format VERSION for loading, default: %d",
      UPROC_DATABASE_LATEST);
    O('S', "store-version", "VERSION",
      "Use database format VERSION for storing, default: %d",
      UPROC_DATABASE_LATEST);
#undef O
}

void progress_cb(double p, void *arg)
{
    (void)arg;
    progress(uproc_stderr, NULL, p);
}

enum nonopt_args { SOURCE, DEST, ARGC };

int main(int argc, char **argv)
{
    uproc_error_set_handler(errhandler_bail, NULL);

    int opt;
    struct ppopts opts = PPOPTS_INITIALIZER;
    make_opts(&opts, argv[0]);
    while ((opt = ppopts_getopt(&opts, argc, argv)) != -1) {
        switch (opt) {
            case 'h':
                ppopts_print(&opts, stdout, 80, 0);
                return EXIT_SUCCESS;
            case 'v':
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'V':
                uproc_features_print(uproc_stdout);
                return EXIT_SUCCESS;
            case 'L':
                if (parse_db_version(optarg, &load_version_)) {
                    fprintf(stderr, "-L argument must be in range (1, %d)\n",
                            UPROC_DATABASE_LATEST);
                    return EXIT_FAILURE;
                }
                break;
            case 'S':
                if (parse_db_version(optarg, &store_version_)) {
                    fprintf(stderr, "-S argument must be in range (1, %d)\n",
                            UPROC_DATABASE_LATEST);
                    return EXIT_FAILURE;
                }
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC) {
        ppopts_print(&opts, stdout, 80, 0);
        return EXIT_FAILURE;
    }
#define IMEX "im"
    char *dir = argv[optind + DEST];
    make_dir(dir);
    char *file = argv[optind + SOURCE];

    char msg[1024];
    snprintf(msg, sizeof msg, "Loading %s", file);
    progress(uproc_stderr, msg, -1);
    uproc_io_stream *stream = open_read(file);
    uproc_database *db =
        uproc_database_unmarshal(stream, load_version_, progress_cb, NULL);
    uproc_io_close(stream);

    snprintf(msg, sizeof msg, "Storing %s", dir);
    progress(uproc_stderr, msg, -1);
    uproc_database_store(db, dir, store_version_, progress_cb, NULL);

    uproc_database_destroy(db);
    return EXIT_SUCCESS;
}
