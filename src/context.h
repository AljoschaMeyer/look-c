#ifndef OO_CONTEXT_H
#define OO_CONTEXT_H

#include "asg.h"
#include "parser.h"
#include "rax.h"
#include "util.h"

typedef enum {
  OO_ERR_NONE, OO_ERR_SYNTAX, OO_ERR_FILE, OO_ERR_IMPORT, OO_ERR_DUP_ID
} OoErrorTag;

typedef struct OoError {
  OoErrorTag tag;
  union {
    ParserError parser;
    Str dup;
  };
} OoError;

typedef struct OoContext {
  // file path of the directory from which to resolve mods
  const char *mods;
  // file path of the directory in which to look for deps
  const char *deps;
  // Set of enabled features, used for conditional compilation
  rax *features;
  // Map from file paths to AsgFiles
  rax *files;
} OoContext;

// Retrieves a parsed file by its path, reading from the fs and parsing if
// necessary. Also filters conditional compilation.
//
// Sets err to OO_ERR_NONE if everything worked, or reports a failure via err.
// In case of failure, this returns NULL, else a pointer to the AsgFile.
//
// `path` is the path of the file on the file system, and must be null-terminated
AsgFile *oo_get_file(OoContext *cx, OoError *err, Str path);

// Delegates to oo_get_file, converting the given stretchy buffer of identifiers
// to a path. The entry in the stretchy buffer must be "dep" or "mod" for this
// to work (errors otherwise).
AsgFile *oo_get_file_ids(OoContext *cx, OoError *err, Str *ids /* stretchy buffer */);

// Populates the items_by_sid and pub_items_by_sid maps.
void oo_init_item_maps(AsgFile *asg, OoContext *cx, OoError *err);

// Frees the features and files raxes, but not their content.
void oo_context_free(OoContext *cx);

#endif
