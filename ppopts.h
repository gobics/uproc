#ifndef PPOPTS_H
#define PPOPTS_H

/** Pretty-print command line options.
 *
 * This module tries its best to print command-line options in a visually
 * pleasing way:
 *
 * - Options, arguments and description texts are horizontally aligned.
 * - Descriptions are automatically wrapped (at word boundaries) to not exceed
 *   the given width (except single words that don't fit)
 * - Print descriptions either on the same line as the options or on the
 *   following one.
 * - Option descriptions containing at least one newline ('\n') character are
 *   always printed literally (on the line after the options), indented by a
 *   few spaces before each line. This allows user-defined formatting mixed
 *   with automatic formatting.
 * - Optionally hide all long descriptions (e.g. if getopt_long() is not
 *   available)
 * - "Header" and "text" items to display text before or between options
 *   (headers are just like text, except that a blank line is printed _before_
 *   instead of _after_ it).
 *
 *
 *
 * Example code:
 *
 *  struct ppopts o;
 *  ppopts_add(&o, 'o', "opt", "A", "Doe something with A.");
 *  ppopts_add(&o, 'f,, "filename", "FILE",
 *             "Read input from FILE. If not specified use standard input.");
 *  ppopts_add(&o, 'd', "debug", "", "Enable debug mode.");
 *  ppopts_add(&o, 'l', "literal, "",
 *             "An option with\n   literal description text.");
 *  ppopts_print(&o, stderr, 75, 0);
 *
 *
 * Output:
 *
 *  -o A     --opt      A       Do something with A.
 *
 *  -f FILE  --filename FILE    Read input from FILE. If not specified use
 *                              standard input.
 *
 *  -d       --debug            Enable debug mode.
 *
 *  -l       --literal
 *      An option with
 *          literal description text.
 *
 *
 * Copyright (c) 2014 Robin Martinjak <robin@rmartinjak.de>
 *
 * Copying and distribution of this file, with or without modification, are
 * permitted in any medium without royalty provided the copyright notice
 * and this notice are preserved.
 */

#include <stdio.h>
#include <limits.h>


/** Always print the descriptions on the next line */
#define PPOPTS_DESC_ON_NEXT_LINE (1 << 0)


#define PPOPTS_LONGOPT_MAX 30
#define PPOPTS_ARGNAME_MAX 10
#define PPOPTS_DESC_MAX 4096
#define PPOPTS_OPTS_MAX 50

enum {
    PPOPTS_HEADER = SCHAR_MAX + 1,
    PPOPTS_TEXT
};

struct ppopts
{
    struct ppopts_opt {
        int shortopt;
        char longopt[PPOPTS_LONGOPT_MAX + 1];
        char argname[PPOPTS_ARGNAME_MAX + 1];
        char desc[PPOPTS_DESC_MAX + 1];
    } options[PPOPTS_OPTS_MAX];
    int maxlen_longopt, maxlen_argname;
    int n;
};


#define PPOPTS_INITIALIZER { {{ 0, "", "", "" }}, 0, 0, 0 }


void ppopts_init(struct ppopts *o);


void ppopts_add(struct ppopts *o, int shortopt, const char *longopt,
                const char *argname, const char *desc, ...);


#define ppopts_add_header(o, ...) \
    ppopts_add((o), PPOPTS_HEADER, "", "", __VA_ARGS__)


#define ppopts_add_text(o, ...) \
    ppopts_add((o), PPOPTS_TEXT, "", "", __VA_ARGS__)


void ppopts_print(struct ppopts *o, FILE *stream, int wrap, int flags);


int ppopts_getopt(struct ppopts *o, int argc, char * const argv[]);
#endif
