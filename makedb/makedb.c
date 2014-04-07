/* uproc-makedb
 * Create a new uproc database.
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

#include <string.h>
#include <time.h>

#include <uproc.h>
#include "makedb.h"

#define PROGNAME "uproc-makedb"

void
print_usage(const char *progname)
{
    fprintf(
        stderr,
        PROGNAME ", version " PACKAGE_VERSION "\n"
        "\n"
        "USAGE: %s [options] MODELDIR SOURCEFILE DESTDIR\n"
        "Build uproc database from the model in MODELDIR and a fasta SOURCEFILE\n"
        "and store it in DESTDIR (which must exist).\n"
        "\n"
        "GENERAL OPTIONS:\n"
        OPT("-h", "--help    ", "     ") "Print this message and exit.\n"
        OPT("-v", "--version ", "     ") "Print version and exit.\n"
        OPT("-c", "--calib   ", "     ") "Only re-calibrate existing DB.\n"
        ,
        progname);
}

static int
load_alphabet(const char *modeldir,
              char alphabet[static UPROC_ALPHABET_SIZE + 1])
{
    char tmp[UPROC_ALPHABET_SIZE + 2];
    uproc_io_stream *stream;
    stream = uproc_io_open("r", UPROC_IO_GZIP, "%s/alphabet", modeldir);
    if (!stream) {
        return -1;
    }
    if (!uproc_io_gets(tmp, sizeof tmp, stream)) {
        return -1;
    }
    alphabet[UPROC_ALPHABET_SIZE] = '\0';
    memcpy(alphabet, tmp, UPROC_ALPHABET_SIZE);
    return 0;
};


int
write_db_info(const char *outdir, const char *infile)
{
    uproc_io_stream *stream;
    time_t now = time(NULL);

    stream = uproc_io_open("w", UPROC_IO_STDIO, "%s/info.txt", outdir);
    if (!stream) {
        return -1;
    }
    uproc_io_printf(stream, "version:    " PACKAGE_VERSION "\n");
    uproc_io_printf(stream, "created:    %s", ctime(&now));
    uproc_io_printf(stream, "input file: %s\n", infile);
    uproc_io_close(stream);
    return 0;
}


int
main(int argc, char **argv)
{
    int res;
    char alphabet[UPROC_ALPHABET_SIZE + 1],
         *modeldir,
         *infile,
         *outdir;
    bool calib_only = false;

    int opt;
    enum args
    {
        MODELDIR, INFILE, OUTDIR,
        ARGC
    };

#define SHORT_OPTS "hvVca:"

#if HAVE_GETOPT_LONG
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "libversion", no_argument,        NULL, 'V' },
        { "calib",      no_argument,        NULL, 'c' },
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
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'V':
                uproc_features_print(uproc_stderr);
                return EXIT_SUCCESS;
            case 'c':
                calib_only = true;
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    modeldir = argv[optind + MODELDIR];
    infile = argv[optind + INFILE];
    outdir = argv[optind + OUTDIR];

    res = load_alphabet(modeldir, alphabet);
    if (res) {
        uproc_perror("error loading model");
        return EXIT_FAILURE;
    }

    if (!calib_only) {
        uproc_idmap *idmap = uproc_idmap_create();
        if (!idmap) {
            uproc_perror("");
            return EXIT_FAILURE;
        }
        res = build_ecurves(infile, outdir, alphabet, idmap);
        if (res) {
            uproc_perror("error building ecurves");
            return EXIT_FAILURE;
        }

        res = uproc_idmap_store(idmap, UPROC_IO_GZIP, "%s/idmap", outdir);
        uproc_idmap_destroy(idmap);
        if (res) {
            uproc_perror("error storing idmap");
            return EXIT_FAILURE;
        }
        res = write_db_info(outdir, infile);
        if (res) {
            uproc_perror("error writing database info");
            return EXIT_FAILURE;
        }
    }

    res = calib(alphabet, outdir, modeldir);
    if (res) {
        uproc_perror("error while calibrating");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
