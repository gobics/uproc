#include <math.h>
#include <errno.h>

#include "test.h"
#include "upro.h"

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
    struct upro_fasta_reader rd;
    unsigned i;
    upro_io_stream *stream;


    stream = upro_io_open(TMPFILE, "w", iotype);
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    i = 300000;
    while (i--) {
        upro_fasta_write(stream, orig_id, orig_c, orig_seq, 20);
    }
    upro_io_close(stream);

    stream = upro_io_open(TMPFILE, "r", iotype);
    if (!stream) {
        FAIL("can't open temporary file: %s", strerror(errno));
    }
    upro_fasta_reader_init(&rd, 8192);
    i = 300000;
    while (i--) {
        upro_fasta_read(stream, &rd);
        assert_str_eq(rd.header, orig_id, "id read correctly");
        assert_str_eq(rd.comment, orig_c, "comment read correctly");
        assert_str_eq(rd.seq, orig_seq, "sequence read correctly");
    }
    upro_io_close(stream);

    upro_fasta_reader_free(&rd);

    return SUCCESS;
}


int fasta(void)
{
    DESC("store and load FASTA sequence");
    return fasta_(UPRO_IO_STDIO);
}

int fasta_gz(void)
{
    DESC("store and load FASTA sequence with gzip compression");
#if HAVE_ZLIB
    return fasta_(UPRO_IO_GZIP);
#else
    SKIP("HAVE_ZLIB not defined");
#endif
}


TESTS_INIT(fasta, fasta_gz);
