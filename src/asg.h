// An abstract syntax graph for look.
// This graph contains pointer into the concrete syntax, and it also holds
// type information. It is one datastructure for everything the compiler
// needs to do.
#ifndef OO_ASG_H
#define OO_ASG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "rax.h"
#include "util.h"

// A datastructure representing the content of a file of oo code.
// It transitively owns all its data, excluding pointers to bindings.
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
typedef struct AsgNS AsgNS;
typedef struct AsgTypeSum AsgTypeSum;
typedef struct AsgSummand AsgSummand;
typedef struct AsgSid AsgSid;
typedef struct AsgPatternId AsgPatternId;
typedef struct AsgItemType AsgItemType;
typedef struct AsgItemVal AsgItemVal;
typedef struct AsgItemFun AsgItemFun;
typedef struct AsgItemFfiVal AsgItemFfiVal;

typedef struct AsgBinding AsgBinding;
typedef struct OoType OoType;

typedef enum {
  OO_TYPE_UNINITIALIZED, // default value before type checking
  OO_TYPE_BINDING,
  OO_TYPE_PTR,
  OO_TYPE_PTR_MUT,
  OO_TYPE_ARRAY,
  OO_TYPE_PRODUCT_REPEATED,
  OO_TYPE_PRODUCT_ANON,
  OO_TYPE_PRODUCT_NAMED,
  OO_TYPE_FUN_ANON,
  OO_TYPE_FUN_NAMED,
  OO_TYPE_SUM,
  OO_TYPE_GENERIC,
  OO_TYPE_APP
} OoTypeTag;

typedef struct OoTypeProductRepeated {
  OoType *inner;
  uint32_t repetitions;
} OoTypeProductRepeated;

typedef struct OoTypeProductNamed {
  OoType *types; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as inners
} OoTypeProductNamed;

typedef struct OoTypeFunAnon {
  OoType *args; // stretchy buffer
  OoType *ret;
} OoTypeFunAnon;

typedef struct OoTypeFunNamed {
  OoType *arg_types; // stretchy buffer
  AsgSid *arg_sids; // stretchy buffer, same length as arg_types
  OoType *ret;
} OoTypeFunNamed;

typedef struct OoTypeGeneric {
  size_t generic_args;
  OoType *inner;
} OoTypeGeneric;

typedef struct OoTypeApp {
  OoTypeGeneric *tlf; // not owning
  OoType *args; // stretchy buffer
} OoTypeApp;

typedef struct OoType {
  OoTypeTag tag;
  union {
    AsgBinding *binding;
    OoType *ptr;
    OoType *ptr_mut;
    OoType *array;
    OoTypeProductRepeated product_repeated;
    OoType *product_anon; // stretchy buffer
    OoTypeProductNamed product_named;
    OoTypeFunAnon fun_anon;
    OoTypeFunNamed fun_named;
    AsgTypeSum *sum;
    OoTypeGeneric generic;
    OoTypeApp app;
    uint32_t arg; // A type argument of an OoTypeGeneric, identified by its index
  };
} OoType;

typedef enum {
  PRIM_U8,
  PRIM_U16,
  PRIM_U32,
  PRIM_U64,
  PRIM_U128,
  PRIM_USIZE,
  PRIM_I8,
  PRIM_I16,
  PRIM_I32,
  PRIM_I64,
  PRIM_I128,
  PRIM_ISIZE,
  PRIM_F32,
  PRIM_F64,
  PRIM_VOID,
  PRIM_BOOL
} AsgPrimitive;

typedef enum {
  BINDING_NONE,
  BINDING_TYPE,
  BINDING_VAL,
  BINDING_NS,
  BINDING_SUM_TYPE,
  BINDING_TYPE_VAR,
  BINDING_PRIMITIVE,
} TagBinding;

typedef struct AsgBindingSum {
  AsgItem *type;
  AsgNS *ns;
} AsgBindingSum;

typedef enum {
  VAL_VAL,
  VAL_FUN,
  VAL_FFI,
  VAL_ARG,
  VAL_PATTERN,
  VAL_SUMMAND
} TagVal;

typedef struct AsgBindingVal {
  bool mut;
  AsgSid *sid;
  AsgType *type; // NULL if no type annoation or fun or summand (TODO does this actually get used?)
  OoType oo_type; // tag OO_TYPE_UNINITIALIZED if no type annotation
  TagVal tag;
  union {
    AsgItemVal *val;
    AsgItemFun *fun;
    AsgItemFfiVal *ffi;
    AsgSid *arg;
    AsgPatternId *pattern;
    AsgSummand *summand;
  };
} AsgBindingVal;

// References to other parts of the ASG.
typedef struct AsgBinding {
  TagBinding tag;
  bool private; // If false, the non-public information of the binding should not be accessed.
  AsgFile *file; // File in which the binding is defined. NULL for directories, mod, dep, primitives, etc.
  union {
    AsgItemType *type;
    AsgBindingVal val;
    AsgNS *ns;
    AsgBindingSum sum;
    AsgSid *type_var;
    AsgPrimitive primitive;
  };
} AsgBinding;

bool is_type_binding(AsgBinding b);

typedef enum {
  NS_FILE,
  NS_DIR,
  NS_MODS,
  NS_DEPS,
  NS_SUM
} TagNS;

// A namespace. These are owned by AsgFiles, sum AsgTypeSums, and by the OoContext (for
// the mod and dep namespace, and for all directories).
typedef struct AsgNS {
  rax *bindings_by_sid;
  rax *pub_bindings_by_sid;
  AsgBinding *bindings; // owning stretchy buffer
  TagNS tag;
  union {
    AsgFile *file;
    AsgTypeSum *sum;
  };
} AsgNS;

void free_ns(AsgNS ns);

// A simple identifier
typedef struct AsgSid {
  Str str;
  AsgBinding binding;
} AsgSid;

typedef struct AsgId {
  Str str;
  AsgSid *sids; // stretchy buffer
  AsgBinding binding;
} AsgId;

typedef struct AsgMacroInv {
  Str str;
  Str name;
  Str args;
} AsgMacroInv;

typedef enum {
  LITERAL_INT,
  LITERAL_FLOAT,
  LITERAL_STRING,
  LITERAL_TRUE,
  LITERAL_FALSE,
  LITERAL_HALT
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
  OP_WRAPPING_PLUS,
  OP_WRAPPING_MINUS,
  OP_WRAPPING_TIMES,
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
  const char *path; // owning
  Str str; // not owning
  AsgItem *items; // owning stretchy buffer
  AsgMeta **attrs; // owning stretchy buffer of owning stretchy buffers, same length as items
  AsgNS ns;
} AsgFile;

// Filters out all items and expressions with cc (conditional compilation)
// attributes whose feature is not in the given rax.
// void oo_filter_cc(AsgFile *asg, rax *features);

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
  AsgFile *asg;
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
  AsgType *types; // stretchy buffer
  AsgSid *sids; // stretchy buffer, same length as types
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
  AsgNS ns;
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
  OoType oo_type;
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
  EXP_WRAPPING_NEGATE,
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
  ASSIGN_SHIFT_R,
  ASSIGN_WRAPPING_PLUS,
  ASSIGN_WRAPPING_MINUS,
  ASSIGN_WRAPPING_TIMES,
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
    AsgExp *exp_wrapping_negate;
    AsgExpBinOp bin_op;
    AsgExpAssign assign;
    AsgPattern val;
    AsgExpValAssign val_assign;
    AsgBlock block;
    AsgExpIf exp_if;
    AsgExpCase exp_case;
    AsgExpWhile exp_while;
    AsgExpLoop exp_loop;
    AsgExp *exp_return; // NULL if no explicit expression is returned
    AsgExp *exp_break; // NULL if no explicit expression is broken
    AsgSid exp_goto;
    AsgSid exp_label;
  };
} AsgExp;

typedef struct AsgItemVal {
  bool mut;
  AsgSid sid;
  AsgType type;
  AsgExp exp;
} AsgItemVal;

typedef struct AsgItemFun {
  AsgSid sid;
  AsgSid *type_args; // stretchy buffer
  AsgSid *arg_sids; //stretchy buffer
  bool *arg_muts; // stretchy buffer, same length as arg_sids
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
  AsgFile *asg;
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
