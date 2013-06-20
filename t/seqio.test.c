#include <math.h>

#include "test.h"
#include "ecurve.h"

#define TMPFILE "t/tmpfile"

void setup(void)
{
}

void teardown(void)
{
}

int fasta_no_filter(void)
{
    char
        *orig_id = "test",
        *orig_c = "a comment",
        *orig_seq = "ACGA-*!CISRHGDUHGIDHGOUARGOWHNSRG!!",
        *id = NULL,
        *c = NULL,
        *seq = NULL;
    size_t id_sz, c_sz, seq_sz;
    unsigned i;
    FILE *stream;
    
    DESC("store and load FASTA sequence without filter");

    stream = fopen(TMPFILE, "w");
    if (!stream) {
        FAIL("can't open temporary file");
    }
    i = 3;
    while (i--) {
        ec_seqio_fasta_print(stream, orig_id, orig_c, orig_seq, 20);
    }
    fclose(stream);

    stream = fopen(TMPFILE, "r");
    if (!stream) {
        FAIL("can't open temporary file");
    }
    i = 3;
    while (i--) {
        ec_seqio_fasta_read(stream, NULL, &id, &id_sz,
                            &c, &c_sz, &seq, &seq_sz);
        assert_str_eq(id, orig_id, "id read correctly");
        assert_str_eq(c, orig_c, "comment read correctly");
        assert_str_eq(seq, orig_seq, "sequence read correctly");
    }
    fclose(stream);

    return SUCCESS;
}

int fasta_filter(void)
{
    char
        *orig_id = "test",
        *orig_c = "a comment",
        *orig_seq = "ACG?+ACU1GUA!_G",
        *expected_seq = "ACG+ACUGUA_G",
        *id = NULL,
        *c = NULL,
        *seq = NULL;
    size_t id_sz, c_sz, seq_sz;
    unsigned i;
    FILE *stream;
    struct ec_seqio_filter filter;

    DESC("store and load FASTA sequence with filter");

    ec_seqio_filter_init(&filter, EC_SEQIO_F_RESET, EC_SEQIO_WARN, "ACGTU", "!?1", NULL);

    stream = fopen(TMPFILE, "w");
    if (!stream) {
        FAIL("can't open temporary file");
    }
    i = 3;
    while (i--) {
        ec_seqio_fasta_print(stream, orig_id, orig_c, orig_seq, 20);
    }
    fclose(stream);

    stream = fopen(TMPFILE, "r");
    if (!stream) {
        FAIL("can't open temporary file");
    }
    i = 3;
    while (i--) {
        ec_seqio_fasta_read(stream, &filter, &id, &id_sz,
                            &c, &c_sz, &seq, &seq_sz);
        assert_str_eq(id, orig_id, "id read correctly");
        assert_str_eq(c, orig_c, "comment read correctly");
        assert_str_eq(seq, expected_seq, "sequence read correctly");
        assert_uint_eq(filter.warnings, 2, "warning indicated");
        assert_uint_eq(filter.ignored, 3, "ignored characters indicated");
    }
    fclose(stream);

    return SUCCESS;
}

TESTS_INIT(fasta_no_filter, fasta_filter);
