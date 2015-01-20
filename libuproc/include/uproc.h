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
 * If you browse the <a href="modules.html">Modules</a>, page, you will see
 * quite a bunch of "modules" called "object uproc_..." or "struct uproc_...".
 * These are the types defined by libuproc and they usually follow the
 * conventions described below.
 *
 * \subsection subsec_opaque Opaque object types
 *
 * Opaque object types are usually defined as forward declarations to structs
 * hidden in the implementation (to ease API compatibility across different
 * versions):
 *
 * \code
 * typedef ... uproc_OBJECT;
 * \endcode
 *
 * \subsubsection subsec_opaque_create Creation
 * Objects of such type are created by calling the corresponding
 * \c uproc_OBJECT_create function, e.g.:
 *
 * \code
 * uproc_bst *tree = uproc_bst_create(...);
 * \endcode
 *
 * \subsubsection subsec_opaque_destroy Destruction
 * Every non-NULL pointer returned by \c uproc_OBJECT_create should be passed to
 * the corresponding \c uproc_OBJECT_destroy:
 *
 * \code
 * uproc_bst_destroy(tree);
 * \endcode
 *
 * Passing NULL to any of the \c uproc_OBJECT_destroy functions is a no-op
 * (and will NOT result in a nullpointer dereference).
 *
 *
 * \subsubsection subsec_opaque_iter Iterator object types
 *
 * Certain objects are \e iterators (the ones whose OBJECT part ends in
 * "iter").  These have a function \c uproc_OBJECT_next that returns 0 if an
 * item was produced, 1 if the iterator is exhausted or -1 on error.
 *
 *
 * \subsection subsec_struct Struct types
 *
 * \subsubsection subsec_struct_init Initialization
 *
 * For a struct type \c struct \c uproc_STRUCT, if there is an initializer
 * macro called \c UPROC_STRUCT_INITIALIZER present, this initializer can be
 * used when defining objects of that type.
 * Alternatively, such a struct can be initialized by passing a pointer to it
 * to the coresponding \c uproc_OBJECT_init function.
 *
 *
 * \subsubsection subsec_struct_free Freeing struct members
 *
 * If the struct contains pointers to allocated storage, they can (and should)
 * be freed using \c uproc_STRUCT_free. The values of affected members will be
 * set to NULL.
 *
 *
 * \subsubsection subsec_struct_copy Deep-copying
 *
 * If there is a corresponding \c uproc_STRUCT_copy function, it should be used
 * to deep-copy the struct.
 *
 *
 * \section sec_multithread Multithreading and reentrancy
 *
 * Except for the following, all parts of the library are reentrant:
 *
 * When using either of ::uproc_stdin, ::uproc_stdout_gz or ::uproc_stderr_gz
 * for the first time, all of them are initialized. A race condition would
 * result in a (minor) memory leak.
 *
 * The \ref sec_error mechanisms are only tested with OpenMP and might not
 * behave correctly if used in conjunction with other threading implementations.
 */

/**
 * \defgroup grp_clf Sequence classification
 * \{
 *   \defgroup grp_clf_prot Protein classification
 *     <!-- protclass.h -->
 *
 *   \defgroup grp_clf_orf ORF translation
 *     <!-- orf.h -->
 *
 *   \defgroup grp_clf_dna DNA classification
 *     <!-- dnaclass.h -->
 * \}
 *
 *
 * \defgroup grp_io Input/output
 * \{
 *   \defgroup grp_io_io General IO
 *     <!-- io.h -->
 *
 *   \defgroup grp_io_seqio Sequence IO
 *     <!-- seqio.h -->
 * \}
 *
 * \defgroup grp_features Info about compile-time features
 *   <!-- features.h -->
 *
 * \defgroup grp_error Error handling
 *   <!-- error.h -->
 *
 *
 * \defgroup grp_datastructs Data structures
 * \{
 *   \defgroup grp_datastructs_list List
 *     <!-- list.h -->
 *
 *   \defgroup grp_datastructs_dict Dictionary
 *     <!-- dict.h -->
 *
 *   \defgroup grp_datastructs_bst Binary search tree
 *     <!-- bst.h -->
 *
 *   \defgroup grp_datastructs_matrix 2D double matrix
 *     <!-- matrix.h -->
 *
 *   \defgroup grp_datastructs_ecurve Evolutionary Curve
 *     <!-- ecurve.h -->
 *
 *   \defgroup grp_datastructs_substmat Amino acid substitution matrix
 *     <!-- substmat.h -->
 *
 *   \defgroup grp_datastructs_idmap ID map
 *     <!-- idmap.h -->
 * \}
 *
 * \defgroup grp_intern Lower-level modules
 * \{
 *   \defgroup grp_intern_common Common definitions
 *     <!-- common.h -->
 *
 *   \defgroup grp_intern_codon Nucleotides and codons
 *     <!-- codon.h -->
 *
 *   \defgroup grp_intern_alpha Amino acid translation alphabets
 *     <!-- alphabet.h -->
 *
 *   \defgroup grp_intern_word Amino acid words
 *     <!-- word.h -->
 * \}
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
#include <uproc/dict.h>
#include <uproc/dnaclass.h>
#include <uproc/ecurve.h>
#include <uproc/error.h>
#include <uproc/features.h>
#include <uproc/idmap.h>
#include <uproc/io.h>
#include <uproc/list.h>
#include <uproc/matrix.h>
#include <uproc/orf.h>
#include <uproc/protclass.h>
#include <uproc/substmat.h>
#include <uproc/seqio.h>
#include <uproc/word.h>
#include <uproc/model.h>
#include <uproc/database.h>

#endif
