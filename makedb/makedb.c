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
#include "ppopts.h"

#define PROGNAME "uproc-makedb"

void
make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
    ppopts_add_text(o, "USAGE: %s [options] MODELDIR SOURCEFILE DESTDIR",
                    progname);

    ppopts_add_text(o,
        "Builds a UProC database from the model in MODELDIR and a FASTA/FASTQ \
        formatted SOURCEFILE and stores it in DESTDIR"
#if !defined(HAVE_MKDIR) && !defined(HAVE__MKDIR)
        " (which must already exist)"
#endif
        ".");
    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help",       "", "Print this message and exit.");
    O('v', "version",    "", "Print version and exit.");
    O('V', "libversion", "", "Print libuproc version/features and exit.");
    O('c', "calib",      "",
      "Re-calibrate existing database (SOURCEFILE will be ignored).");
#undef O
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
    uproc_io_printf(stream, "version:    " UPROC_VERSION "\n");
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

    enum nonopt_args
    {
        MODELDIR, INFILE, OUTDIR,
        ARGC
    };

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
            case 'c':
                calib_only = true;
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC) {
        ppopts_print(&opts, stderr, 80, 0);
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
        make_dir(outdir);
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
