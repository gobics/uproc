#ifndef MAKEDB_H
#define MAKEDB_H

#include <uproc.h>

/* from build_ecurves.c */
int build_ecurves(const char *infile, const char *outdir, const char *alphabet,
                  uproc_idmap *idmap);

/* from calib.c */
int calib(const char *alphabet, const char *dbdir, const char *modeldir);
#endif
