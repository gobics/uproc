#ifndef UPROC_OPT_H
#define UPROC_OPT_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_GETOPT_LONG
#define _GNU_SOURCE
#include <getopt.h>
#define OPT(shortopt, longopt, arg) shortopt " " arg "  " longopt " " arg "    "
#else
#include <unistd.h>
#define OPT(shortopt, longopt, arg) shortopt " " arg "    "
#define getopt_long(argc, argv, shortopts, longopts, longindex) \
    getopt(argc, argv, shortopts)
#endif

#include <stdio.h>

static void
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

#endif
