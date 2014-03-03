#ifndef UPROC_H
#define UPROC_H

/** \mainpage
 *
 * \tableofcontents
 *
 * \section sec_error Error handling
 *
 * Unless noted otherwise, functions return the following values:
 *
 * \li  0 for success or -1 for failure if the return type is int.
 * \li  Non-NULL for success or NULL for failure if the return type is a pointer
 *      type.
 *
 * In case of failure, functions set a global (thread-local if compiled with
 * OpenMP) error code and message, see error.h for details on how to retrieve
 * the error.
 *
 *
 * \section sec_objects Types and objects
 *
 * \subsection subsec_opaque Opaque types
 *
 * Opaque types are generally defined as
 * \code
 * typedef uproc_OBJECT_s uproc_OBJECT;
 * \endcode
 *
 * Objects of such type are created by calling the corresponding
 * \c uproc_OBJECT_create function, e.g.:
 *
 * \code
 * uproc_bst *tree = uproc_bst_create(...);
 * \endcode
 *
 * Every non-NULL pointer returned by \c uproc_OBJECT_create should be passed to
 * the corresponding \c uproc_OBJECT_destroy:
 *
 * \code
 * uproc_bst_destroy(tree);
 * \endcode
 *
 * Passing NULL to any of the * \c uproc_OBJECT_destroy functions is a no-op
 * (and will NOT result in a nullpointer dereference).
 *
 *
 * \subsection subsec_struct Struct types
 *
 * For a struct type \c struct \c uproc_OBJECT, if there is an initializer macro
 * called \c UPROC_OBJECT_INITIALIZER present, this initializer should be used
 * when defining objects of that type.
 *
 *
 * \section sec_multithread Multithreading and reentrancy
 *
 * Except for the following, all parts of the library are reentrant:
 *
 * When using either of ::uproc_stdin, ::uproc_stdin_gz, ::uproc_stdout_gz or
 * ::uproc_stderr_gz for the first time, all of them are initialized. A race
 * condition would result in a (minor) memory leak.
 *
 * The \ref sec_error mechanisms are only tested with OpenMP and might not
 * behave correctly if used in conjunction with other threading implementations.
 */

/**
 * \defgroup grp_clf Classification
 * @{
 *   \defgroup grp_clf_dna DNA classification
 *     dnaclass.h
 *   \defgroup grp_clf_prot Protein classification
 *     protclass.h
 * @}
 *
 *
 * \defgroup grp_io Input/output
 * @{
 *   \defgroup grp_io_io General IO
 *     io.h
 *   \defgroup grp_io_seqio Sequence IO
 *     seqio.h
 * @}
 *
 * \defgroup grp_intern Lower-level modules
 * @{
 *   \defgroup grp_intern_codon Nucleotides and codons
 *     codon.h
 *   \defgroup grp_intern_alpha Amino acid translation alphabets
 *     alphabet.h
 *   \defgroup grp_intern_word Amino acid words
 *     word.h
 * @}
 *
 */

/** \file uproc.h
 *
 * Includes all other headers.
 *
 */

#include <uproc/alphabet.h>
#include <uproc/bst.h>
#include <uproc/codon.h>
#include <uproc/common.h>
#include <uproc/ecurve.h>
#include <uproc/error.h>
#include <uproc/idmap.h>
#include <uproc/io.h>
#include <uproc/orf.h>
#include <uproc/substmat.h>
#include <uproc/seqio.h>
#include <uproc/word.h>

#include <uproc/protclass.h>
#include <uproc/dnaclass.h>
#endif
