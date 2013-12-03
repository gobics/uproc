#ifndef MAKEDB_H
#define MAKEDB_H

#include <uproc.h>

/* from build_ecurves.c */
int build_ecurves(const char *infile, const char *outdir, const char *alphabet,
                  struct uproc_idmap *idmap);

/* from calib.c */
int calib(char *dbdir, char *modeldir);

/* from progress.c */
void progress(const char *label, double percent);
#endif
