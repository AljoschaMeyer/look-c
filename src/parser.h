#ifndef OO_PARSER_H
#define OO_PARSER_H

#include <stddef.h>

#include "asg.h"
#include "lexer.h"

typedef enum {
  ERR_NONE,
  ERR_SID,
  ERR_ID
} OoTagError;

typedef struct OoError {
  OoTagError tag;
  TokenType tt;
} OoError;

// All parser functions return how many bytes of the input they consumed.
// The first argument is the string (null-terminated) from which to parse.
// Errors are signaled via the second argument.
// The actual parsed data is populated via the third argument.

size_t parse_file(const char *src, OoError *err, AsgFile *data);

size_t parse_meta(const char *src, OoError *err, AsgMeta *data);
size_t parse_meta_nested(const char *src, OoError *err, AsgMetaNested *data);

size_t parse_item(const char *src, OoError *err, AsgItem *data);

size_t parse_use_tree(const char *src, OoError *err, AsgUseTree *data);
size_t parse_use_tree_rename(const char *src, OoError *err, AsgUseTreeRename *data);
size_t parse_use_tree_branch(const char *src, OoError *err, AsgUseTreeBranch *data);

size_t parse_item_type(const char *src, OoError *err, AsgItemType *data);
size_t parse_type(const char *src, OoError *err, AsgType *data);
size_t parse_type_product_repeated(const char *src, OoError *err, AsgTypeProductRepeated *data);
size_t parse_type_product_anon(const char *src, OoError *err, AsgTypeProductAnon *data);
size_t parse_type_product_named(const char *src, OoError *err, AsgTypeProductNamed *data);
size_t parse_type_fun_anon(const char *src, OoError *err, AsgTypeFunAnon *data);
size_t parse_type_fun_named(const char *src, OoError *err, AsgTypeFunNamed *data);
size_t parse_type_app_anon(const char *src, OoError *err, AsgTypeAppAnon *data);
size_t parse_type_app_named(const char *src, OoError *err, AsgTypeAppNamed *data);
size_t parse_type_generic(const char *src, OoError *err, AsgTypeGeneric *data);
size_t parse_type_sum(const char *src, OoError *err, AsgTypeSum *data);

size_t parse_summand(const char *src, OoError *err, AsgSummand *data);
size_t parse_summand_anon(const char *src, OoError *err, AsgSummandAnon *data);
size_t parse_summand_named(const char *src, OoError *err, AsgSummandNamed *data);

size_t parse_item_val(const char *src, OoError *err, AsgItemVal *data);
size_t parse_exp(const char *src, OoError *err, AsgExp *data);
size_t parse_exp_array_index(const char *src, OoError *err, AsgExpArrayIndex *data);
size_t parse_exp_product_repeated(const char *src, OoError *err, AsgExpProductRepeated *data);
size_t parse_exp_product_anon(const char *src, OoError *err, AsgExpProductAnon *data);
size_t parse_exp_product_named(const char *src, OoError *err, AsgExpProductNamed *data);
size_t parse_exp_product_access_anon(const char *src, OoError *err, AsgExpProductAccessAnon *data);
size_t parse_exp_product_access_named(const char *src, OoError *err, AsgExpProductAccessNamed *data);
size_t parse_exp_fun_app_anon(const char *src, OoError *err, AsgExpFunAppAnon *data);
size_t parse_exp_fun_app_named(const char *src, OoError *err, AsgExpFunAppNamed *data);
size_t parse_exp_type_app_anon(const char *src, OoError *err, AsgExpTypeAppAnon *data);
size_t parse_exp_type_app_named(const char *src, OoError *err, AsgExpTypeAppNamed *data);
size_t parse_exp_cast(const char *src, OoError *err, AsgExpCast *data);
size_t parse_exp_bin_op(const char *src, OoError *err, AsgExpBinOp *data);
size_t parse_exp_assign(const char *src, OoError *err, AsgExpAssign *data);
size_t parse_exp_val(const char *src, OoError *err, AsgExpVal *data);
size_t parse_exp_block(const char *src, OoError *err, AsgExpBlock *data);
size_t parse_exp_if(const char *src, OoError *err, AsgExpIf *data);
size_t parse_exp_while(const char *src, OoError *err, AsgExpWhile *data);
size_t parse_exp_case(const char *src, OoError *err, AsgExpCase *data);
size_t parse_exp_loop(const char *src, OoError *err, AsgExpLoop *data);

size_t parse_lvalue(const char *src, OoError *err, AsgLValue *data);
size_t parse_lvalue_array_index(const char *src, OoError *err, AsgLValueArrayIndex *data);
size_t parse_lvalue_product_access_anon(const char *src, OoError *err, AsgLValueProductAccessAnon *data);
size_t parse_lvalue_product_access_named(const char *src, OoError *err, AsgLValueProductAccessNamed *data);

size_t parse_pattern(const char *src, OoError *err, AsgPattern *data);
size_t parse_pattern_id(const char *src, OoError *err, AsgPatternId *data);
size_t parse_pattern_product_anon(const char *src, OoError *err, AsgPatternProductAnon *data);
size_t parse_pattern_product_named(const char *src, OoError *err, AsgPatternProductNamed *data);
size_t parse_pattern_summand_anon(const char *src, OoError *err, AsgPatternSummandAnon *data);
size_t parse_pattern_summand_named(const char *src, OoError *err, AsgPatternSummandNamed *data);

size_t parse_patter_irref(const char *src, OoError *err, AsgPatternIrref *data);
size_t parse_pattern_irref_product_anon(const char *src, OoError *err, AsgPatternIrrefProductAnon *data);
size_t parse_pattern_irref_product_named(const char *src, OoError *err, AsgPatternIrrefProductNamed *data);

size_t parse_item_fun(const char *src, OoError *err, AsgItemFun *data);

size_t parse_item_ffi_include(const char *src, OoError *err, AsgItemFfiInclude *data);
size_t parse_item_ffi_val(const char *src, OoError *err, AsgItemFfiVal *data);

size_t parse_id(const char *src, OoError *err, AsgId *data);
size_t parse_sid(const char *src, OoError *err, AsgSid *data);
size_t parse_macro_inv(const char *src, OoError *err, AsgMacroInv *data);
size_t parse_literal(const char *src, OoError *err, AsgLiteral *data);

size_t parse_repeat(const char *src, OoError *err, AsgRepeat *data);
size_t parse_repeat_bin_op(const char *src, OoError *err, AsgRepeatBinOp *data);

// The free_inner_foo functions free the child nodes of their argument (but
// not the argument itself).

void free_inner_id(AsgId data);

#endif
