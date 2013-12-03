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

#include "uproc.h"

#define PROGNAME "uproc-makedb"
#define ALPHA_DEFAULT "AGSTPKRQEDNHYWFMLIVC"


/* from build_ecurves.c */
int build_ecurves(const char *infile, const char *outdir, const char *alphabet,
                  struct uproc_idmap *idmap);

/* from calib.c */
int calib(char *dbdir, char *modeldir);



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
#ifdef EXPORT
        OPT("-n", "--nocompress") "Don't store using gzip compression\n"
#endif
        ,
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

    int opt;
    enum args
    {
        MODELDIR, INFILE, OUTDIR,
        ARGC
    };

#define SHORT_OPTS "hva:"

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
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

    res = build_ecurves(infile, outdir, alphabet, &idmap);
    if (res) {
        return EXIT_FAILURE;
    }

    res = uproc_idmap_store(&idmap, UPROC_IO_GZIP, "%s/idmap");
    if (res) {
        return EXIT_FAILURE;
    }

    res = calib(outdir, modeldir);
    if (res) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
