// An abstract syntax graph for look.
// This graph contains pointer into the concrete syntax, and it also holds
// type information. It's one datastructure for everything the compiler
// needs to do.
#ifndef OO_ASG_H
#define OO_ASG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "rax.h"

struct AsgMeta;

typedef struct AsgItem AsgItem;
typedef struct AsgMeta AsgMeta;
typedef struct AsgType AsgType;
typedef struct AsgExp AsgExp;
typedef struct AsgRepeat AsgRepeat;
typedef struct AsgLValue AsgLValue;
typedef struct AsgPattern AsgPattern;

typedef enum {
  BINDING_NONE, // the sid does not resolve to a binding (because it defines one)
  BINDING_ERROR, // binding resolution failed
  BINDING_ITEM_USE,
  BINDING_ITEM_USE_RENAME,
  BINDING_ITEM_TYPE,
  BINDING_ITEM_VAL,
  BINDING_ITEM_FUN,
  BINDING_ITEM_FFI,
  BINDING_PATTERN,
  BINDING_FIELD,
  BINDING_TYPE_ARG
  // TODO local val? fn args? named products?
  // Add these as necessary when implementing binding resolution.
} BindingType;

// A simple identifier
typedef struct AsgSid {
  const char *src;
  size_t len;
  BindingType bt;
  void *binding;
} AsgSid;

typedef struct AsgId {
  const char *src;
  size_t len;
  AsgSid *sids;
} AsgId;

typedef struct AsgMacroInv {
  const char *src;
  size_t len;
  const char *name;
  size_t name_len;
  const char *args;
  size_t args_len;
} AsgMacroInv;

typedef enum {
  LITERAL_INT,
  LITERAL_FLOAT,
  LITERAL_STRING
} TagLiteral;

typedef struct AsgLiteral {
  const char *src;
  size_t len;
  TagLiteral tag;
} AsgLiteral;

typedef enum {
  OP_PLUS,
  OP_MINUS,
  OP_TIMES,
  OP_DIV,
  OP_MOD,
  OP_SHIFT_L,
  OP_SHIFT_R,
  OP_OR,
  OP_AND,
  OP_XOR,
  OP_LAND,
  OP_LOR,
  OP_EQ,
  OP_NEQ,
  OP_GT,
  OP_GET,
  OP_LT,
  OP_LET,
} AsgBinOp;

typedef enum {
  REPEAT_INT,
  REPEAT_MACRO,
  REPEAT_SIZE_OF,
  REPEAT_ALIGN_OF,
  REPEAT_BIN_OP
} TagRepeat;

typedef struct AsgRepeatBinOp {
  AsgBinOp op;
  AsgRepeat *lhs;
  AsgRepeat *rhs;
} AsgRepeatBinOp;

typedef struct AsgRepeat {
  const char *src;
  size_t len;
  TagRepeat tag;
  union {
    char integer; // always an integer literal
    AsgMacroInv macro;
    AsgType *size_of;
    AsgType *align_of;
    AsgRepeatBinOp bin_op;
  };
} AsgRepeat;

// Root node of the asg.
typedef struct AsgFile {
  const char *src;
  size_t len;
  AsgItem *items;
  size_t item_len;
  rax *items_by_id; // stores `AsgItem`s TODO remove this, keep analysis separate?
} AsgFile;

// TODO functions: parse, typecheck (context?), free (does not free source string)

typedef enum {
  META_NULLARY,
  META_UNARY,
  META_NESTED
} TagMeta;

typedef struct AsgMetaNested {
  const char *src;
  size_t len;
  AsgMeta *inner;
  size_t inner_len;
} AsgMetaNested;

typedef struct AsgMeta {
  const char *src;
  size_t len;
  TagMeta tag;
  char *name;
  size_t name_len;
  union {
    char nullary; // never used
    AsgLiteral unary;
    AsgMetaNested nested;
  };
} AsgMeta;

typedef enum {
  ITEM_USE,
  ITEM_TYPE,
  ITEM_VAL,
  ITEM_FUN, // unlike a regular val, this does not need a type annotation
  ITEM_FFI_INCLUDE,
  ITEM_FFI_VAL
} TagItem;

typedef enum {
  USE_TREE_LEAF,
  USE_TREE_RENAME,
  USE_TREE_BRANCH
} TagUseTree;

typedef struct AsgUseTreeRename {
  const char *src;
  size_t len;
  AsgSid sid;
  AsgSid new_sid;
} AsgUseTreeRename;

typedef struct AsgUseTreeBranch {
  const char *src;
  size_t len;
  AsgSid sid;
  AsgSid *children;
  size_t children_len;
} AsgUseTreeBranch;

typedef struct AsgUseTree {
  const char *src;
  size_t len;
  TagUseTree tag;
  union {
    AsgSid leaf;
    AsgUseTreeRename rename;
    AsgUseTreeBranch branch;
  };
} AsgUseTree;

typedef enum {
  TYPE_ID,
  TYPE_MACRO,
  TYPE_PTR,
  TYPE_PTR_MUT,
  TYPE_ARRAY,
  TYPE_PRODUCT_REPEATED,
  TYPE_PRODUCT_ANON,
  TYPE_PRODUCT_NAMED,
  TYPE_FUN_ANON,
  TYPE_FUN_NAMED,
  TYPE_APP_ANON,
  TYPE_APP_NAMED,
  TYPE_GENERIC,
  TYPE_SUM
} TagType;

typedef struct AsgTypeProductRepeated {
  AsgType *inner;
  AsgRepeat repeat;
} AsgTypeProductRepeated;

typedef struct AsgTypeProductNamed {
  AsgType *types; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as inners
} AsgTypeProductNamed;

typedef struct AsgTypeFunAnon {
  AsgType *args; // stretchy buffer
  AsgType *ret;
} AsgTypeFunAnon;

typedef struct AsgTypeFunNamed {
  AsgType *arg_types; // stretchy buffer
  AsgSid *arg_sids; // stretchy buffer, same length as args
  AsgType *ret;
} AsgTypeFunNamed;

typedef struct AsgTypeAppAnon {
  AsgId tlf; // The type-level function that is applied
  AsgType *args; // stretchy buffer
} AsgTypeAppAnon;

typedef struct AsgTypeAppNamed {
  AsgId tlf;
  AsgType *types;
  AsgSid *sids; // same length as args
  size_t args_len;
} AsgTypeAppNamed;

typedef struct AsgTypeGeneric {
  AsgSid *args; // stretchy buffer
  AsgType *inner;
} AsgTypeGeneric;

typedef enum { SUMMAND_ANON, SUMMAND_NAMED } TagSummand;

typedef struct AsgSummandNamed {
  AsgType *inners; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as inners
} AsgSummandNamed;

typedef struct AsgSummand {
  const char *src;
  size_t len;
  TagSummand tag;
  AsgSid sid;
  union {
    AsgType *anon; // stretchy buffer
    AsgSummandNamed named;
  };
} AsgSummand;

typedef struct AsgTypeSum {
  bool pub; // Whether the tags are visible (opaque type if false)
  AsgSummand *summands; // stretchy buffer
} AsgTypeSum;

typedef struct AsgType {
  const char *src;
  size_t len;
  TagType tag;
  union {
    AsgId id;
    AsgMacroInv macro;
    AsgType *ptr;
    AsgType *ptr_mut;
    AsgType *array;
    AsgTypeProductRepeated product_repeated;
    AsgType *product_anon; // stretchy_buffer
    AsgTypeProductNamed product_named;
    AsgTypeFunAnon fun_anon;
    AsgTypeFunNamed fun_named;
    AsgTypeAppAnon app_anon;
    AsgTypeAppNamed app_named;
    AsgTypeGeneric generic;
    AsgTypeSum sum;
  };
} AsgType;

typedef struct AsgItemType {
  AsgSid sid;
  AsgType type;
} AsgItemType;

typedef enum {
  PATTERN_ID,
  PATTERN_BLANK,
  PATTERN_LITERAL,
  PATTERN_PTR,
  PATTERN_PRODUCT_ANON,
  PATTERN_PRODUCT_NAMED,
  PATTERN_SUMMAND_ANON,
  PATTERN_SUMMAND_NAMED
} TagPattern;

typedef struct AsgPatternId {
  bool mut;
  AsgSid sid;
  AsgType *type; // may be null if no type annotation is present
} AsgPatternId;

typedef struct AsgPatternProductNamed {
  AsgPattern *inners; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as inners
} AsgPatternProductNamed;

typedef struct AsgPatternSummandAnon {
  AsgId id;
  AsgPattern *fields; // stretchy buffer
} AsgPatternSummandAnon;

typedef struct AsgPatternSummandNamed {
  AsgId id;
  AsgPattern *fields; // stretchy buffer
  AsgSid *sids; // stretchy buffer,  same length as fields
} AsgPatternSummandNamed;

typedef struct AsgPattern {
  const char *src;
  size_t len;
  TagPattern tag;
  union {
    AsgPatternId id;
    AsgLiteral lit;
    AsgPattern *ptr;
    AsgPattern *product_anon; // stretchy buffer
    AsgPatternProductNamed product_named;
    AsgPatternSummandAnon summand_anon;
    AsgPatternSummandNamed summand_named;
  };
} AsgPattern;

typedef enum {
  EXP_ID,
  EXP_MACRO,
  EXP_LITERAL,
  EXP_REF,
  EXP_REF_MUT,
  EXP_DEREF,
  EXP_DEREF_MUT,
  EXP_ARRAY,
  EXP_ARRAY_INDEX,
  EXP_PRODUCT_REPEATED,
  EXP_PRODUCT_ANON,
  EXP_PRODUCT_NAMED,
  EXP_PRODUCT_ACCESS_ANON,
  EXP_PRODUCT_ACCESS_NAMED,
  EXP_FUN_APP_ANON,
  EXP_FUN_APP_NAMED,
  EXP_TYPE_APP_ANON,
  EXP_TYPE_APP_NAMED,
  EXP_CAST,
  EXP_SIZE_OF,
  EXP_ALIGN_OF,
  EXP_NOT,
  EXP_NEGATE,
  EXP_BIN_OP,
  EXP_ASSIGN,
  EXP_VAL,
  EXP_IF,
  EXP_CASE,
  EXP_WHILE,
  EXP_LOOP,
  EXP_RETURN,
  EXP_BREAK,
  EXP_BLOCK,
  EXP_GOTO,
  EXP_LABEL
} ExpTag;

typedef struct AsgExpArrayIndex {
  AsgExp *arr;
  AsgExp *index;
} AsgExpArrayIndex;

typedef struct AsgExpProductRepeated {
  AsgExp *inner;
  AsgRepeat repeat;
} AsgExpProductRepeated;

typedef struct AsgExpProductNamed {
  AsgExp *inners; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as inners
} AsgExpProductNamed;

typedef struct AsgExpProductAccessAnon {
  AsgExp *inner;
  long field;
} AsgExpProductAccessAnon;

typedef struct AsgExpProductAccessNamed {
  AsgExp *inner;
  AsgSid field;
} AsgExpProductAccessNamed;

typedef struct AsgExpFunAppAnon {
  AsgExp *fun;
  AsgExp *args;
  size_t args_len;
} AsgExpFunAppAnon;

typedef struct AsgExpFunAppNamed {
  AsgExp *fun;
  AsgExp *args;
  AsgSid *names; // same length as args
  size_t args_len;
} AsgExpFunAppNamed;

typedef struct AsgExpTypeAppAnon {
  AsgId type;
  AsgType *args;
  size_t args_len;
} AsgExpTypeAppAnon;

typedef struct AsgExpTypeAppNamed {
  AsgId type;
  AsgType *args;
  AsgSid *names; // same length as args
  size_t args_len;
} AsgExpTypeAppNamed;

typedef struct AsgExpCast {
  AsgExp *inner;
  AsgType *type;
} AsgExpCast;

typedef struct AsgExpBinOp {
  AsgBinOp op;
  AsgExp *lhs;
  AsgExp *rhs;
} AsgExpBinOp;

typedef enum {
  ASSIGN_REGULAR,
  ASSIGN_PLUS,
  ASSIGN_MINUS,
  ASSIGN_TIMES,
  ASSIGN_DIV,
  ASSIGN_MOD,
  ASSIGN_AND,
  ASSIGN_OR,
  ASSIGN_XOR,
  ASSIGN_SHIFT_L,
  ASSIGN_SHIFT_R
} AsgAssignOp;

typedef struct AsgExpAssign {
  AsgAssignOp op;
  AsgExp *lhs;
  AsgExp *rhs;
} AsgExpAssign;

typedef struct AsgExpVal {
  AsgPattern lhs;
  AsgExp *rhs; // is null for declarations without direct assignment
} AsgExpVal;

typedef struct AsgExpBlock {
  AsgExp *exps;
  AsgMeta **annotations; // same length as exps
  size_t *annotation_lens; // same length as annotations
  size_t exps_len;
} AsgExpBlock;

typedef struct AsgExpIf {
  AsgExp *cond;
  AsgExpBlock if_block;
  AsgExpBlock *else_block; // is null if no else is present
} AsgExpIf;

typedef struct AsgExpWhile {
  AsgExp *cond;
  AsgExpBlock block;
} AsgExpWhile;

typedef struct AsgExpCase {
  AsgExp *matcher;
  AsgPattern *patterns;
  AsgExpBlock *blocks; // same length as patterns
  size_t pattern_len;
} AsgExpCase;

typedef struct AsgExpLoop {
  AsgExp *matcher;
  AsgPattern *patterns;
  AsgExpBlock *blocks; // same length as patterns
  size_t pattern_len;
} AsgExpLoop;

typedef struct AsgExp {
  const char *src;
  size_t len;
  ExpTag tag;
  union {
    AsgId id;
    AsgMacroInv macro;
    AsgLiteral lit;
    AsgExp *ref;
    AsgExp *ref_mut;
    AsgExp *deref;
    AsgExp *deref_mut;
    AsgExp *array;
    AsgExpArrayIndex array_index;
    AsgExpProductRepeated product_repeated;
    AsgExp *product_anon; // stretchy buffer
    AsgExpProductNamed product_named;
    AsgExpProductAccessAnon product_access_anon;
    AsgExpProductAccessNamed product_access_named;
    AsgExpFunAppAnon fun_app_anon;
    AsgExpFunAppNamed fun_app_named;
    AsgExpTypeAppAnon type_app_anon;
    AsgExpTypeAppNamed type_app_named;
    AsgExpCast cast;
    AsgType *size_of;
    AsgType *align_of;
    AsgExp *exp_not;
    AsgExp *exp_negate;
    AsgExpBinOp bin_op;
    AsgExpAssign assign;
    AsgExpVal val;
    AsgExpIf exp_if;
    AsgExpCase exp_case;
    AsgExpWhile exp_while;
    AsgExpLoop loop;
    AsgExp *exp_return;
    AsgExp *exp_break;
    AsgExpBlock block;
    AsgSid exp_goto;
    AsgSid exp_label;
  };
} AsgExp;

typedef struct AsgItemVal {
  bool mut;
  AsgSid sid;
  AsgExp exp;
} AsgItemVal;

typedef struct AsgItemFun {
  AsgSid *type_args;
  size_t type_args_len;
  AsgSid *arg_sids;
  AsgType *arg_types; // same length as arg_sids
  size_t arg_sids_len;
  AsgType *ret; // may be null if return type is omitted
  AsgExpBlock *body;
} AsgItemFun;

typedef struct AsgItemFfiInclude {
  char *include; // e.g. <stdio.h> or "asg.h"
  size_t include_len;
} AsgItemFfiInclude;

typedef struct AsgItemFfiVal {
  bool mut;
  AsgSid sid;
  AsgType type;
} AsgItemFfiVal;

typedef struct AsgItem {
  const char *src;
  size_t len;
  TagItem tag;
  AsgMeta *attrs;
  size_t attrs_len;
  bool pub; // ignored for ffi_includes
  union {
    AsgUseTree use;
    AsgItemType type;
    AsgItemVal val;
    AsgItemFun fun;
    AsgItemFfiInclude ffi_include;
    AsgItemFfiVal ffi_val;
  };
} AsgItem;

#endif
