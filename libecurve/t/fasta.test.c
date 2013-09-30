#include <math.h>
#include <errno.h>

#include "test.h"
#include "ecurve.h"

#define TMPFILE "t/tmpfile"

void setup(void)
{
}

void teardown(void)
{
}

int fasta(void)
{
    char
        *orig_id = "test",
        *orig_c = "a comment",
        *orig_seq = "ACGA-*!CISRHGDUHGIDHGOUARGOWHNSRG!!";
    struct ec_fasta_reader rd;
    unsigned i;
    FILE *stream;


    DESC("store and load FASTA sequence");

    stream = fopen(TMPFILE, "w");
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    i = 3;
    while (i--) {
        ec_fasta_write(stream, orig_id, orig_c, orig_seq, 20);
    }
    fclose(stream);

    stream = fopen(TMPFILE, "r");
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    ec_fasta_reader_init(&rd, 8192);
    i = 3;
    while (i--) {
        ec_fasta_read(stream, &rd);
        assert_str_eq(rd.header, orig_id, "id read correctly");
        assert_str_eq(rd.comment, orig_c, "comment read correctly");
        assert_str_eq(rd.seq, orig_seq, "sequence read correctly");
    }
    fclose(stream);

    return SUCCESS;
}

TESTS_INIT(fasta);
