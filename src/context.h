#ifndef OO_CONTEXT_H
#define OO_CONTEXT_H

#include "asg.h"
#include "parser.h"
#include "rax.h"
#include "util.h"

typedef enum {
  OO_ERR_NONE, OO_ERR_SYNTAX, OO_ERR_FILE, OO_ERR_CYCLIC_USES, OO_ERR_DUP_ID_ITEM,
  OO_ERR_DUP_ID_ITEM_USE, OO_ERR_INVALID_BRANCH, OO_ERR_NONEXISTING_SID_USE

  // OO_ERR_FILE, OO_ERR_IMPORT, OO_ERR_DUP_ID,
  // OO_ERR_NONEXISTING_SID, OO_ERR_INVALID_BRANCH, OO_ERR_UNEXPECTED_DEP,
  // OO_ERR_UNEXPECTED_MOD
} OoErrorTag;

// TODO add AsgFile, file path, and the asg node
typedef struct OoError {
  OoErrorTag tag;
  union {
    ParserError parser; // OO_ERR_SYNTAX
    const char *file; // OO_ERR_FILE, OO_ERR_CYCLIC_USES
    AsgItem *dup_item;
    AsgUseTree *dup_item_use;
    AsgUseTree *nonexisting_sid_use;
    AsgUseTree *invalid_branch;
  };
} OoError;

// Owns all data related to the multiple asgs of a parser run (including the asgs themselves).
typedef struct OoContext {
  // File path of the directory from which to resolve mods
  const char *mods;
  // File path of the directory in which to look for deps
  const char *deps;
  // Owning stretchy buffer of owned pointers to all files
  AsgFile **files;
  // Owning stretchy buffer of owned pointers to all implicit namespaces:
  // - dirs[0] is the `mod` namespace
  // - dirs[1] is the `dep` namespace
  // - remaining entries correspond to directories in the sources
  AsgNS **dirs;
  // Owning stretchy buffer of the source text of all files
  char **sources;
} OoContext;

// Initializes a context, but does not perform any parsing yet.
void oo_cx_init(OoContext *cx, const char *mods, const char *deps);

// Parses all files relevant to the current mod and deps, and adds them to cx->files.
// Applies conditional compilation based on the given features.
// Also creates the cx->dirs.
void oo_cx_parse(OoContext *cx, OoError *err, rax *features);

// Resolves the top-level bindings of all files.
void oo_cx_coarse_bindings(OoContext *cx, OoError *err);

// Frees all data owned by the context, including all parsed files and all namespaces.
// The `mods` and `deps` directory paths are not freed.
void oo_cx_free(OoContext *cx);

#endif
