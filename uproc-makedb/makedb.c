#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_GETOPT_LONG
#define _GNU_SOURCE
#include <getopt.h>
#define OPT(shortopt, longopt, arg) shortopt " " arg "  " longopt " " arg "    "
#else
#define OPT(shortopt, longopt, arg) shortopt " " arg "    "
#endif
#include <unistd.h>

#include <uproc.h>
#include "makedb.h"

#define PROGNAME "uproc-makedb"
#define ALPHA_DEFAULT "AGSTPKRQEDNHYWFMLIVC"

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
        "USAGE: %s [options] MODELDIR SOURCEFILE DESTDIR\n"
        "Build uproc database from the model in MODELDIR and a fasta SOURCEFILE\n"
        "and store it in DESTDIR (which must exist).\n"
        "\n"
        "GENERAL OPTIONS:\n"
        OPT("-h", "--help    ", "     ") "Print this message and exit.\n"
        OPT("-v", "--version ", "     ") "Print version and exit.\n"
        OPT("-c", "--calib   ", "     ") "Only re-calibrate existing DB.\n"
        OPT("-a", "--alphabet", "ALPHA") "Use alphabet ALPHA instead of \""
            ALPHA_DEFAULT "\"\n",
        progname);
}

int
main(int argc, char **argv)
{
    int res;
    struct uproc_idmap idmap = UPROC_IDMAP_INITIALIZER;
    char *alphabet = ALPHA_DEFAULT,
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

#define SHORT_OPTS "hvca:"

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "calib",      no_argument,        NULL, 'c' },
        { "alphabet",   no_argument,        NULL, 'a' },
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
            case 'c':
                calib_only = true;
                break;
            case 'a':
                alphabet = optarg;
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    modeldir = argv[optind + MODELDIR];
    infile = argv[optind + INFILE];
    outdir = argv[optind + OUTDIR];

    if (!calib_only) {
        res = build_ecurves(infile, outdir, alphabet, &idmap);
        if (res) {
            return EXIT_FAILURE;
        }

        res = uproc_idmap_store(&idmap, UPROC_IO_GZIP, "%s/idmap", outdir);
        if (res) {
            uproc_perror("error storing idmap");
            return EXIT_FAILURE;
        }
    }

    res = calib(alphabet, outdir, modeldir);
    if (res) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
