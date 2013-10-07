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

int fasta_(int iotype)
{
    char
        *orig_id = "test",
        *orig_c = "a comment",
        *orig_seq = "ACGA-*!CISRHGDUHGIDHGOUARGOWHNSRG!!";
    struct ec_fasta_reader rd;
    unsigned i;
    ec_io_stream *stream;


    stream = ec_io_open(TMPFILE, "w", iotype);
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    i = 300000;
    while (i--) {
        ec_fasta_write(stream, orig_id, orig_c, orig_seq, 20);
    }
    ec_io_close(stream);

    stream = ec_io_open(TMPFILE, "r", iotype);
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    ec_fasta_reader_init(&rd, 8192);
    i = 300000;
    while (i--) {
        ec_fasta_read(stream, &rd);
        assert_str_eq(rd.header, orig_id, "id read correctly");
        assert_str_eq(rd.comment, orig_c, "comment read correctly");
        assert_str_eq(rd.seq, orig_seq, "sequence read correctly");
    }
    ec_io_close(stream);

    return SUCCESS;
}


int fasta(void)
{
    DESC("store and load FASTA sequence");
    return fasta_(EC_IO_STDIO);
}

int fasta_gz(void)
{
    DESC("store and load FASTA sequence with gzip compression");
#if HAVE_ZLIB
    return fasta_(EC_IO_GZIP);
#else
    SKIP("HAVE_ZLIB not defined");
#endif
}


TESTS_INIT(fasta, fasta_gz);
