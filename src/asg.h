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
#include "util.h"

typedef struct AsgFile AsgFile;
typedef struct AsgItem AsgItem;
typedef struct AsgMeta AsgMeta;
typedef struct AsgType AsgType;
typedef struct AsgExp AsgExp;
typedef struct AsgRepeat AsgRepeat;
typedef struct AsgLValue AsgLValue;
typedef struct AsgPattern AsgPattern;
typedef struct AsgBlock AsgBlock;
typedef struct AsgUseTree AsgUseTree;
typedef struct AsgMod AsgMod;

typedef enum {
  BINDING_TYPE,
  BINDING_VAL,
  BINDING_FUN,
  BINDING_FFI_VAL,
  BINDING_MOD
} TagBinding;

typedef struct AsgBinding {
  TagBinding tag;
  union {
    AsgItem *type;
    AsgItem *val;
    AsgItem *fun;
    AsgItem *ffi_val;
    AsgMod *mod;
  };
} AsgBinding;

// Not syntactic, part of the name resolution. Each file owns one of these,
// and the OoContext holds one per directory.
typedef struct AsgMod {
  rax *bindings_by_sid;
  rax *pub_bindings_by_sid;
  AsgBinding *bindings; // stretchy buffer owning all bindings
} AsgMod;

// A simple identifier
typedef struct AsgSid {
  Str str;
} AsgSid;

typedef struct AsgId {
  Str str;
  AsgSid *sids; // stretchy buffer
} AsgId;

typedef struct AsgMacroInv {
  Str str;
  Str name;
  Str args;
} AsgMacroInv;

typedef enum {
  LITERAL_INT,
  LITERAL_FLOAT,
  LITERAL_STRING
} TagLiteral;

typedef struct AsgLiteral {
  Str str;
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
  Str str;
  TagRepeat tag;
  union {
    AsgMacroInv macro;
    AsgType *size_of;
    AsgType *align_of;
    AsgRepeatBinOp bin_op;
  };
} AsgRepeat;

// Root node of the asg.
typedef struct AsgFile {
  Str str;
  AsgItem *items; // stretchy buffer
  AsgMeta **attrs; // stretchy buffer of stretchy buffers, same length as items
  AsgMod mod;
  bool did_init_mod;
} AsgFile;

// Filters out all items and expressions with cc (conditional compilation)
// attributes whose feature is not in the given rax.
void oo_filter_cc(AsgFile *asg, rax *features);

typedef enum {
  META_NULLARY,
  META_UNARY,
  META_NESTED
} TagMeta;

typedef struct AsgMeta {
  Str str;
  TagMeta tag;
  Str name;
  union {
    AsgLiteral unary;
    AsgMeta *nested; // stretchy buffer
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

typedef struct AsgUseTree {
  Str str;
  TagUseTree tag;
  AsgSid sid;
  union {
    AsgSid rename;
    AsgUseTree* branch; // stretchy buffer
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
  Str str;
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
  Str str;
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
  Str str;
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

typedef struct AsgBlock {
  Str str;
  AsgExp *exps; // stretchy buffer
  AsgMeta **attrs; // stretchy buffer of stretchy buffers, same length as exps
} AsgBlock;

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
  EXP_CAST,
  EXP_SIZE_OF,
  EXP_ALIGN_OF,
  EXP_NOT,
  EXP_NEGATE,
  EXP_BIN_OP,
  EXP_ASSIGN,
  EXP_VAL,
  EXP_VAL_ASSIGN,
  EXP_BLOCK,
  EXP_IF,
  EXP_CASE,
  EXP_WHILE,
  EXP_LOOP,
  EXP_RETURN,
  EXP_BREAK,
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
  unsigned long field;
} AsgExpProductAccessAnon;

typedef struct AsgExpProductAccessNamed {
  AsgExp *inner;
  AsgSid field;
} AsgExpProductAccessNamed;

typedef struct AsgExpFunAppAnon {
  AsgExp *fun;
  AsgExp *args; // stretchy buffer
} AsgExpFunAppAnon;

typedef struct AsgExpFunAppNamed {
  AsgExp *fun;
  AsgExp *args; // stretchy buffer
  AsgSid *sids; // stretchy buffer,  same length as args
} AsgExpFunAppNamed;

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

typedef struct AsgExpValAssign {
  AsgPattern lhs;
  AsgExp *rhs;
} AsgExpValAssign;

typedef struct AsgExpIf {
  AsgExp *cond;
  AsgBlock if_block;
  AsgBlock else_block; // no else is represented as an empty AsgBlock
} AsgExpIf;

typedef struct AsgExpWhile {
  AsgExp *cond;
  AsgBlock block;
} AsgExpWhile;

typedef struct AsgExpCase {
  AsgExp *matcher;
  AsgPattern *patterns; // stretchy buffer
  AsgBlock *blocks; // stretchy buffer, same length as patterns
} AsgExpCase;

typedef struct AsgExpLoop {
  AsgExp *matcher;
  AsgPattern *patterns; // stretchy buffer
  AsgBlock *blocks; // stretchy buffer, same length as patterns
} AsgExpLoop;

typedef struct AsgExp {
  Str str;
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
    AsgExpCast cast;
    AsgType *size_of;
    AsgType *align_of;
    AsgExp *exp_not;
    AsgExp *exp_negate;
    AsgExpBinOp bin_op;
    AsgExpAssign assign;
    AsgPattern val;
    AsgExpValAssign val_assign;
    AsgBlock block;
    AsgExpIf exp_if;
    AsgExpCase exp_case;
    AsgExpWhile exp_while;
    AsgExpLoop exp_loop;
    AsgExp *exp_return;
    AsgExp *exp_break;
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
  AsgSid sid;
  AsgSid *type_args; // stretchy buffer
  AsgSid *arg_sids; //stretchy buffer
  AsgType *arg_types; // stretchy buffer, same length as arg_sids
  AsgType ret; // empty anon product if return type is omitted in the syntax
  AsgBlock body;
} AsgItemFun;

typedef struct AsgItemFfiInclude {
  Str include;
} AsgItemFfiInclude;

typedef struct AsgItemFfiVal {
  bool mut;
  AsgSid sid;
  AsgType type;
} AsgItemFfiVal;

typedef struct AsgItem {
  Str str;
  TagItem tag;
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
