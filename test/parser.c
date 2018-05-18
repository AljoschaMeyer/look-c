#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/stretchy_buffer.h"
#include "../src/lexer.h"
#include "../src/parser.h"

void test_sid() {
  const char *src = "abc";
  AsgSid data;
  OoError err;
  size_t l = parse_sid(src, &err, &data);

  assert(l == 3);
  assert(err.tag == ERR_NONE);
  assert(data.src == src);
  assert(data.len == 3);
}

void test_id() {
  const char *src = "";
  AsgId data;
  OoError err;
  size_t l = parse_id(src, &err, &data);
  assert(err.tag != ERR_NONE);

  src = "abc";
  l = parse_id(src, &err, &data);
  assert(l == 3);
  assert(err.tag == ERR_NONE);
  assert(sb_count(data.sids) == 1);
  assert(data.sids[0].src == src);
  free_inner_id(data);

  src = "abc:: def ::ghi";
  l = parse_id(src, &err, &data);
  assert(l == 15);
  assert(err.tag == ERR_NONE);
  assert(sb_count(data.sids) == 3);
  assert(data.sids[0].src == src);
  assert(data.sids[0].len == 3);
  assert(data.sids[1].src == src + 6);
  assert(data.sids[0].len == 3);
  assert(data.sids[0].len == 3);
  assert(data.sids[2].src == src + 12);
  free_inner_id(data);
}

void test_macro_inv() {
  const char *src = "$abc()";
  AsgMacroInv data;
  OoError err;
  size_t l;

  l = parse_macro_inv(src, &err, &data);
  assert(l == 6);
  assert(err.tag == ERR_NONE);
  assert(data.name_len == 3);
  assert(data.name == src + 1);
  assert(data.args_len == 0);
  assert(data.args == src + 5);

  src = "$abc(())";
  l = parse_macro_inv(src, &err, &data);
  assert(l == 8);
  assert(err.tag == ERR_NONE);
  assert(data.name_len == 3);
  assert(data.name == src + 1);
  assert(data.args_len == 2);
  assert(data.args == src + 5);
}

void test_literal() {
  const char *src = "42";
  AsgLiteral data;
  OoError err;
  size_t l;

  l = parse_literal(src, &err, &data);
  assert(l == 2);
  assert(err.tag == ERR_NONE);
  assert(data.src == src);
  assert(data.len == 2);
  assert(data.tag == LITERAL_INT);

  src = "0.0";
  l = parse_literal(src, &err, &data);
  assert(l == 3);
  assert(err.tag == ERR_NONE);
  assert(data.src == src);
  assert(data.len == 3);
  assert(data.tag == LITERAL_FLOAT);

  src = "\"abc\"";
  l = parse_literal(src, &err, &data);
  assert(l == 5);
  assert(err.tag == ERR_NONE);
  assert(data.src == src);
  assert(data.len == 5);
  assert(data.tag == LITERAL_STRING);
}

void test_repeat(void) {
  char *src = "42 + $foo()";
  OoError err;
  AsgRepeat data;

  assert(parse_repeat(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == REPEAT_BIN_OP);
  assert(data.len == strlen(src));
  assert(data.bin_op.lhs->tag == REPEAT_INT);
  assert(data.bin_op.lhs->len == 2);
  assert(data.bin_op.rhs->tag == REPEAT_MACRO);
  assert(data.bin_op.rhs->macro.name == src + 6);
  assert(data.bin_op.rhs->macro.name_len == 3);

  free_inner_repeat(data);
}

void test_type(void) {
  char *src = "abc::def";
  OoError err;
  AsgType data;

  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == TYPE_ID);
  assert(sb_count(data.id.sids) == 2);
  free_inner_type(data);

  src = "$foo()";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_MACRO);
  assert(data.len == strlen(src));
  free_inner_type(data);

  src = "@a";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PTR);
  assert(data.len == strlen(src));
  assert(data.ptr->src == src + 1);
  assert(data.ptr->len == 1);
  assert(data.ptr->tag == TYPE_ID);
  free_inner_type(data);

  src = "~@a";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PTR_MUT);
  assert(data.len == strlen(src));
  assert(data.ptr_mut->src == src + 1);
  assert(data.ptr_mut->len == 2);
  assert(data.ptr_mut->tag == TYPE_PTR);
  free_inner_type(data);

  src = "[@a]";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_ARRAY);
  assert(data.len == strlen(src));
  assert(data.array->src == src + 1);
  assert(data.array->len == 2);
  assert(data.array->tag == TYPE_PTR);
  free_inner_type(data);

  src = "(@A; 42)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_REPEATED);
  assert(data.len == strlen(src));
  assert(data.product_repeated.inner->tag == TYPE_PTR);
  assert(data.product_repeated.inner->src == src + 1);
  assert(data.product_repeated.inner->len == 2);
  assert(data.product_repeated.repeat.tag == REPEAT_INT);
  free_inner_type(data);

  src = "( )";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 0);
  free_inner_type(data);

  src = "(A)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 1);
  assert(data.product_anon[0].tag == TYPE_ID);
  assert(data.product_anon[0].src == src + 1);
  assert(data.product_anon[0].len == 1);
  free_inner_type(data);

  src = "(A, @B)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 2);
  assert(data.product_anon[0].tag == TYPE_ID);
  assert(data.product_anon[0].src == src + 1);
  assert(data.product_anon[0].len == 1);
  assert(data.product_anon[1].tag == TYPE_PTR);
  assert(data.product_anon[1].src == src + 4);
  assert(data.product_anon[1].len == 3);
  free_inner_type(data);

  src = "() -> @A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_FUN_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.fun_anon.args) == 0);
  assert(data.fun_anon.ret->tag == TYPE_PTR);
  assert(data.fun_anon.ret->src == src + 6);
  assert(data.fun_anon.ret->len == 3);
  free_inner_type(data);

  src = "(@A) -> ()";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_FUN_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.fun_anon.args) == 1);
  assert(data.fun_anon.args[0].tag == TYPE_PTR);
  assert(data.fun_anon.args[0].src == src + 1);
  assert(data.fun_anon.args[0].len == 2);
  assert(data.fun_anon.ret->tag == TYPE_PRODUCT_ANON);
  assert(sb_count(data.fun_anon.ret->product_anon) == 0);
  free_inner_type(data);

  src = "(a: A)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.types) == 1);
  assert(data.product_named.types[0].tag == TYPE_ID);
  assert(data.product_named.types[0].src == src + 4);
  assert(data.product_named.types[0].len == 2);
  assert(sb_count(data.product_named.sids) == 1);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  free_inner_type(data);

  src = "(a: A, b: @B)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.types) == 2);
  assert(data.product_named.types[0].tag == TYPE_ID);
  assert(data.product_named.types[0].src == src + 4);
  assert(data.product_named.types[0].len == 2);
  assert(data.product_named.types[1].tag == TYPE_PTR);
  assert(data.product_named.types[1].src == src + 10);
  assert(data.product_named.types[1].len == 3);
  assert(sb_count(data.product_named.sids) == 2);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  assert(data.product_named.sids[1].src == src + 7);
  assert(data.product_named.sids[1].len == 1);
  free_inner_type(data);

  src = "(a: @A) -> @A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_FUN_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.fun_named.arg_types) == 1);
  assert(data.fun_named.arg_types[0].tag == TYPE_PTR);
  assert(data.fun_named.arg_types[0].src == src + 4);
  assert(data.fun_named.arg_types[0].len == 3);
  assert(data.fun_named.ret->tag == TYPE_PTR);
  assert(data.fun_named.ret->len == 3);
  free_inner_type(data);

  src = "a<A>";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_APP_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.app_anon.tlf.sids) == 1);
  assert(sb_count(data.app_anon.args) == 1);
  assert(data.app_anon.args[0].tag == TYPE_ID);
  assert(data.app_anon.args[0].src == src + 2);
  assert(data.app_anon.args[0].len == 1);
  free_inner_type(data);

  src = "a<A, @B>";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_APP_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.app_anon.tlf.sids) == 1);
  assert(sb_count(data.app_anon.args) == 2);
  assert(data.app_anon.args[0].tag == TYPE_ID);
  assert(data.app_anon.args[0].src == src + 2);
  assert(data.app_anon.args[0].len == 1);
  assert(data.app_anon.args[1].tag == TYPE_PTR);
  assert(data.app_anon.args[1].src == src + 5);
  assert(data.app_anon.args[1].len == 3);
  free_inner_type(data);

  src = "a<a = A>";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_APP_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.app_named.tlf.sids) == 1);
  assert(sb_count(data.app_named.types) == 1);
  assert(data.app_named.types[0].tag == TYPE_ID);
  assert(data.app_named.types[0].src == src + 6);
  assert(data.app_named.types[0].len == 2);
  assert(sb_count(data.app_named.sids) == 1);
  assert(data.app_named.sids[0].src == src + 2);
  assert(data.app_named.sids[0].len == 1);
  free_inner_type(data);

  src = "a<a = A, b = @B>";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_APP_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.app_named.tlf.sids) == 1);
  assert(sb_count(data.app_named.types) == 2);
  assert(data.app_named.types[0].tag == TYPE_ID);
  assert(data.app_named.types[0].src == src + 6);
  assert(data.app_named.types[0].len == 2);
  assert(data.app_named.types[1].tag == TYPE_PTR);
  assert(data.app_named.types[1].src == src + 13);
  assert(data.app_named.types[1].len == 3);
  assert(sb_count(data.app_named.sids) == 2);
  assert(data.app_named.sids[0].src == src + 2);
  assert(data.app_named.sids[0].len == 1);
  assert(data.app_named.sids[1].src == src + 9);
  assert(data.app_named.sids[1].len == 1);
  free_inner_type(data);

  src = "<A> => @A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_GENERIC);
  assert(data.len == strlen(src));
  assert(sb_count(data.generic.args) == 1);
  assert(data.generic.args[0].src == src + 1);
  assert(data.generic.inner->tag == TYPE_PTR);
  assert(data.generic.inner->len == 3);
  free_inner_type(data);

  src = "<A, B> => @A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_GENERIC);
  assert(data.len == strlen(src));
  assert(sb_count(data.generic.args) == 2);
  assert(data.generic.args[0].src == src + 1);
  assert(data.generic.args[1].src == src + 4);
  assert(data.generic.inner->tag == TYPE_PTR);
  assert(data.generic.inner->len == 3);
  free_inner_type(data);

  src = "| A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_SUM);
  assert(data.len == strlen(src));
  assert(!data.sum.pub);
  assert(sb_count(data.sum.summands) == 1);
  assert(data.sum.summands[0].tag == SUMMAND_ANON);
  assert(data.sum.summands[0].sid.src == src + 2);
  assert(data.sum.summands[0].sid.len == 1);
  assert(sb_count(data.sum.summands[0].anon) == 0);
  free_inner_type(data);

  src = "pub | A(@A) | B(b: @Z)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_SUM);
  assert(data.len == strlen(src));
  assert(data.sum.pub);
  assert(sb_count(data.sum.summands) == 2);
  assert(data.sum.summands[0].tag == SUMMAND_ANON);
  assert(data.sum.summands[0].sid.src == src + 6);
  assert(data.sum.summands[0].sid.len == 1);
  assert(sb_count(data.sum.summands[0].anon) == 1);
  assert(data.sum.summands[0].anon[0].tag == TYPE_PTR);
  assert(data.sum.summands[0].anon[0].src == src + 8);
  assert(data.sum.summands[0].anon[0].len == 2);
  assert(data.sum.summands[1].tag == SUMMAND_NAMED);
  assert(data.sum.summands[1].sid.src == src + 14);
  assert(data.sum.summands[1].sid.len == 1);
  assert(sb_count(data.sum.summands[1].named.inners) == 1);
  assert(sb_count(data.sum.summands[1].named.sids) == 1);
  assert(data.sum.summands[1].named.inners[0].tag == TYPE_PTR);
  assert(data.sum.summands[1].named.inners[0].src == src + 19);
  assert(data.sum.summands[1].named.inners[0].len == 3);
  assert(data.sum.summands[1].named.sids[0].src == src + 16);
  assert(data.sum.summands[1].named.sids[0].len == 1);
  free_inner_type(data);
}

void test_pattern(void) {
  char *src = "_";
  OoError err;
  AsgPattern data;

  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_BLANK);
  free_inner_pattern(data);

  src = "mut abc";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_ID);
  assert(data.id.mut);
  assert(data.id.sid.src == src + 4);
  assert(data.id.sid.len == 3);
  assert(data.id.type == NULL);
  free_inner_pattern(data);

  src = "a: @A";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_ID);
  assert(!data.id.mut);
  assert(data.id.sid.src == src);
  assert(data.id.sid.len == 1);
  assert(data.id.type->tag == TYPE_PTR);
  free_inner_pattern(data);

  src = "42";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 2);
  assert(data.lit.tag == LITERAL_INT);
  free_inner_pattern(data);

  src = "0.0";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 3);
  assert(data.lit.tag == LITERAL_FLOAT);
  free_inner_pattern(data);

  src = "\"abc\"";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 5);
  assert(data.lit.tag == LITERAL_STRING);
  free_inner_pattern(data);

  src = "@a: @A";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PTR);
  assert(data.ptr->tag == PATTERN_ID);
  assert(!data.ptr->id.mut);
  assert(data.ptr->id.sid.src == src + 1);
  assert(data.ptr->id.sid.len == 1);
  assert(data.ptr->id.type->tag == TYPE_PTR);
  free_inner_pattern(data);

  src = "()";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PRODUCT_ANON);
  assert(sb_count(data.product_anon) == 0);
  free_inner_pattern(data);

  src = "(_)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PRODUCT_ANON);
  assert(sb_count(data.product_anon) == 1);
  free_inner_pattern(data);

  src = "(_, _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PRODUCT_ANON);
  assert(sb_count(data.product_anon) == 2);
  free_inner_pattern(data);

  src = "(a = _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PRODUCT_NAMED);
  assert(sb_count(data.product_anon) == 1);
  free_inner_pattern(data);

  src = "(a = _, b = _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_PRODUCT_NAMED);
  assert(sb_count(data.product_anon) == 2);
  free_inner_pattern(data);

  src = "| a";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_SUMMAND_ANON);
  assert(sb_count(data.summand_anon.id.sids) == 1);
  assert(sb_count(data.summand_anon.fields) == 0);
  free_inner_pattern(data);

  src = "| a(_)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_SUMMAND_ANON);
  assert(sb_count(data.summand_anon.id.sids) == 1);
  assert(sb_count(data.summand_anon.fields) == 1);
  free_inner_pattern(data);

  src = "| a(_, _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_SUMMAND_ANON);
  assert(sb_count(data.summand_anon.id.sids) == 1);
  assert(sb_count(data.summand_anon.fields) == 2);
  free_inner_pattern(data);

  src = "| a(b = _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_SUMMAND_NAMED);
  assert(sb_count(data.summand_anon.id.sids) == 1);
  assert(sb_count(data.summand_anon.fields) == 1);
  free_inner_pattern(data);

  src = "| a(b = _, c = _)";
  assert(parse_pattern(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == PATTERN_SUMMAND_NAMED);
  assert(sb_count(data.summand_anon.id.sids) == 1);
  assert(sb_count(data.summand_anon.fields) == 2);
  free_inner_pattern(data);
}

void test_exp(void) {
  char *src = "abc::def";
  OoError err;
  AsgExp data;

  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == EXP_ID);
  assert(sb_count(data.id.sids) == 2);
  free_inner_exp(data);

  src = "$foo()";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_MACRO);
  assert(data.len == strlen(src));
  free_inner_exp(data);

  src = "42";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == EXP_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 2);
  assert(data.lit.tag == LITERAL_INT);
  free_inner_exp(data);

  src = "0.0";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == EXP_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 3);
  assert(data.lit.tag == LITERAL_FLOAT);
  free_inner_exp(data);

  src = "\"abc\"";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == EXP_LITERAL);
  assert(data.lit.src == src);
  assert(data.lit.len == 5);
  assert(data.lit.tag == LITERAL_STRING);
  free_inner_exp(data);

  src = "@a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_REF);
  assert(data.len == strlen(src));
  assert(data.ref->src == src + 1);
  assert(data.ref->len == 1);
  assert(data.ref->tag == EXP_ID);
  free_inner_exp(data);

  src = "~@a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_REF_MUT);
  assert(data.len == strlen(src));
  assert(data.ref_mut->src == src + 1);
  assert(data.ref_mut->len == 2);
  assert(data.ref_mut->tag == EXP_REF);
  free_inner_exp(data);

  src = "[@a]";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ARRAY);
  assert(data.len == strlen(src));
  assert(data.array->src == src + 1);
  assert(data.array->len == 2);
  assert(data.array->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@A; 42)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_REPEATED);
  assert(data.len == strlen(src));
  assert(data.product_repeated.inner->tag == EXP_REF);
  assert(data.product_repeated.inner->src == src + 1);
  assert(data.product_repeated.inner->len == 2);
  assert(data.product_repeated.repeat.tag == REPEAT_INT);
  free_inner_exp(data);

  src = "( )";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 0);
  free_inner_exp(data);

  src = "(A)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 1);
  assert(data.product_anon[0].tag == EXP_ID);
  assert(data.product_anon[0].src == src + 1);
  assert(data.product_anon[0].len == 1);
  free_inner_exp(data);

  src = "(A, @B)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ANON);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_anon) == 2);
  assert(data.product_anon[0].tag == EXP_ID);
  assert(data.product_anon[0].src == src + 1);
  assert(data.product_anon[0].len == 1);
  assert(data.product_anon[1].tag == EXP_REF);
  assert(data.product_anon[1].src == src + 4);
  assert(data.product_anon[1].len == 3);
  free_inner_exp(data);

  src = "(a = A)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.inners) == 1);
  assert(data.product_named.inners[0].tag == EXP_ID);
  assert(data.product_named.inners[0].src == src + 5);
  assert(data.product_named.inners[0].len == 2);
  assert(sb_count(data.product_named.sids) == 1);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  free_inner_exp(data);

  src = "(a = A, b = @B)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.inners) == 2);
  assert(data.product_named.inners[0].tag == EXP_ID);
  assert(data.product_named.inners[0].src == src + 5);
  assert(data.product_named.inners[0].len == 2);
  assert(data.product_named.inners[1].tag == EXP_REF);
  assert(data.product_named.inners[1].src == src + 12);
  assert(data.product_named.inners[1].len == 3);
  assert(sb_count(data.product_named.sids) == 2);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  assert(data.product_named.sids[1].src == src + 8);
  assert(data.product_named.sids[1].len == 1);
  free_inner_exp(data);

  src = "sizeof(@a)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_SIZE_OF);
  assert(data.len == strlen(src));
  assert(data.size_of->src == src + 7);
  assert(data.size_of->len == 2);
  assert(data.size_of->tag == TYPE_PTR);
  free_inner_exp(data);

  src = "alignof(@a)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ALIGN_OF);
  assert(data.len == strlen(src));
  assert(data.align_of->src == src + 8);
  assert(data.align_of->len == 2);
  assert(data.align_of->tag == TYPE_PTR);
  free_inner_exp(data);

  src = "!@a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_NOT);
  assert(data.len == strlen(src));
  assert(data.exp_not->src == src + 1);
  assert(data.exp_not->len == 2);
  assert(data.exp_not->tag == EXP_REF);
  free_inner_exp(data);

  src = "-@a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_NEGATE);
  assert(data.len == strlen(src));
  assert(data.exp_negate->src == src + 1);
  assert(data.exp_negate->len == 2);
  assert(data.exp_negate->tag == EXP_REF);
  free_inner_exp(data);

  src = "val a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_VAL);
  assert(data.len == strlen(src));
  assert(data.val.src == src + 4);
  assert(data.val.len == 2);
  assert(data.val.tag == PATTERN_ID);
  free_inner_exp(data);

  src = "val a = @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_VAL_ASSIGN);
  assert(data.len == strlen(src));
  assert(data.val_assign.lhs.src == src + 4);
  assert(data.val_assign.lhs.len == 2);
  assert(data.val_assign.lhs.tag == PATTERN_ID);
  assert(data.val_assign.rhs->src == src + 8);
  assert(data.val_assign.rhs->len == 3);
  assert(data.val_assign.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "{}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BLOCK);
  assert(data.len == strlen(src));
  assert(sb_count(data.block.exps) == 0);
  assert(sb_count(data.block.attrs) == 0);
  free_inner_exp(data);

  src = "{a}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BLOCK);
  assert(data.len == strlen(src));
  assert(sb_count(data.block.exps) == 1);
  assert(data.block.exps[0].tag == EXP_ID);
  assert(sb_count(data.block.attrs) == 1);
  assert(sb_count(data.block.attrs[0]) == 0);
  free_inner_exp(data);

  src = "{a; b}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BLOCK);
  assert(data.len == strlen(src));
  assert(sb_count(data.block.exps) == 2);
  assert(data.block.exps[0].tag == EXP_ID);
  assert(data.block.exps[1].tag == EXP_ID);
  assert(sb_count(data.block.attrs) == 2);
  assert(sb_count(data.block.attrs[0]) == 0);
  assert(sb_count(data.block.attrs[1]) == 0);
  free_inner_exp(data);

  src = "{#[foo]#[bar]a}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BLOCK);
  assert(data.len == strlen(src));
  assert(sb_count(data.block.exps) == 1);
  assert(data.block.exps[0].tag == EXP_ID);
  assert(sb_count(data.block.attrs) == 1);
  assert(sb_count(data.block.attrs[0]) == 2);
  free_inner_exp(data);

  src = "{a; #[foo]#[bar]b}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BLOCK);
  assert(data.len == strlen(src));
  assert(sb_count(data.block.exps) == 2);
  assert(data.block.exps[0].tag == EXP_ID);
  assert(data.block.exps[1].tag == EXP_ID);
  assert(sb_count(data.block.attrs) == 2);
  assert(sb_count(data.block.attrs[0]) == 0);
  assert(sb_count(data.block.attrs[1]) == 2);
  free_inner_exp(data);

  src = "if a {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_IF);
  assert(data.len == strlen(src));
  assert(data.exp_if.cond->tag == EXP_ID);
  assert(sb_count(data.exp_if.if_block.exps) == 0);
  assert(sb_count(data.exp_if.else_block.exps) == 0);
  free_inner_exp(data);

  src = "if a {} else {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_IF);
  assert(data.len == strlen(src));
  assert(data.exp_if.cond->tag == EXP_ID);
  assert(sb_count(data.exp_if.if_block.exps) == 0);
  assert(sb_count(data.exp_if.else_block.exps) == 0);
  free_inner_exp(data);

  src = "if a {} else if b {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_IF);
  assert(data.len == strlen(src));
  assert(data.exp_if.cond->tag == EXP_ID);
  assert(sb_count(data.exp_if.if_block.exps) == 0);
  assert(sb_count(data.exp_if.else_block.exps) == 1);
  assert(data.exp_if.else_block.exps[0].tag == EXP_IF);
  assert(data.exp_if.else_block.exps[0].exp_if.cond->tag == EXP_ID);
  assert(sb_count(data.exp_if.else_block.exps[0].exp_if.if_block.exps) == 0);
  assert(sb_count(data.exp_if.else_block.exps[0].exp_if.else_block.exps) == 0);
  free_inner_exp(data);

  src = "while a {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_WHILE);
  assert(data.len == strlen(src));
  assert(data.exp_while.cond->tag == EXP_ID);
  assert(sb_count(data.exp_while.block.exps) == 0);
  free_inner_exp(data);

  src = "case a {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_CASE);
  assert(data.len == strlen(src));
  assert(data.exp_case.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_case.patterns) == 0);
  free_inner_exp(data);

  src = "case a {_{}}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_CASE);
  assert(data.len == strlen(src));
  assert(data.exp_case.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_case.patterns) == 1);
  assert(sb_count(data.exp_case.blocks) == 1);
  free_inner_exp(data);

  src = "case a {_{}_{}}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_CASE);
  assert(data.len == strlen(src));
  assert(data.exp_case.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_case.patterns) == 2);
  assert(sb_count(data.exp_case.blocks) == 2);
  free_inner_exp(data);

  src = "loop a {}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_LOOP);
  assert(data.len == strlen(src));
  assert(data.exp_loop.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_loop.patterns) == 0);
  free_inner_exp(data);

  src = "loop a {_{}}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_LOOP);
  assert(data.len == strlen(src));
  assert(data.exp_loop.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_loop.patterns) == 1);
  assert(sb_count(data.exp_loop.blocks) == 1);
  free_inner_exp(data);

  src = "loop a {_{}_{}}";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_LOOP);
  assert(data.len == strlen(src));
  assert(data.exp_loop.matcher->tag == EXP_ID);
  assert(sb_count(data.exp_loop.patterns) == 2);
  assert(sb_count(data.exp_loop.blocks) == 2);
  free_inner_exp(data);

  src = "return @a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_RETURN);
  assert(data.len == strlen(src));
  assert(data.exp_return->src == src + 7);
  assert(data.exp_return->len == 3);
  assert(data.exp_return->tag == EXP_REF);
  free_inner_exp(data);

  src = "break @a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BREAK);
  assert(data.len == strlen(src));
  assert(data.exp_break->src == src + 6);
  assert(data.exp_break->len == 3);
  assert(data.exp_break->tag == EXP_REF);
  free_inner_exp(data);

  src = "goto a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_GOTO);
  assert(data.len == strlen(src));
  assert(data.exp_goto.src == src + 5);
  assert(data.exp_goto.len == 1);
  free_inner_exp(data);

  src = "label a";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_LABEL);
  assert(data.len == strlen(src));
  assert(data.exp_label.src == src + 6);
  assert(data.exp_label.len == 1);
  free_inner_exp(data);

  src = "a@";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_DEREF);
  assert(data.len == strlen(src));
  assert(data.deref->src == src);
  assert(data.deref->len == 1);
  assert(data.deref->tag == EXP_ID);
  free_inner_exp(data);

  src = "a@@";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_DEREF);
  assert(data.len == strlen(src));
  assert(data.deref->src == src);
  assert(data.deref->len == 2);
  assert(data.deref->tag == EXP_DEREF);
  free_inner_exp(data);

  src = "a~";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_DEREF_MUT);
  assert(data.len == strlen(src));
  assert(data.deref_mut->src == src);
  assert(data.deref_mut->len == 1);
  assert(data.deref_mut->tag == EXP_ID);
  free_inner_exp(data);

  src = "a[@b]";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ARRAY_INDEX);
  assert(data.len == strlen(src));
  assert(data.array_index.arr->tag == EXP_ID);
  assert(data.array_index.index->tag == EXP_REF);
  free_inner_exp(data);

  src = "a.42";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ACCESS_ANON);
  assert(data.len == strlen(src));
  assert(data.product_access_anon.inner->tag == EXP_ID);
  assert(data.product_access_anon.field == 42);
  free_inner_exp(data);

  src = "a.42foo";
  assert(parse_exp(src, &err, &data) == 4);
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ACCESS_ANON);
  assert(data.len == 4);
  assert(data.product_access_anon.inner->tag == EXP_ID);
  assert(data.product_access_anon.field == 42);
  free_inner_exp(data);

  src = "a.b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_PRODUCT_ACCESS_NAMED);
  assert(data.len == strlen(src));
  assert(data.product_access_named.inner->tag == EXP_ID);
  assert(data.product_access_named.field.src == src + 2);
  free_inner_exp(data);

  src = "a()";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_FUN_APP_ANON);
  assert(data.len == strlen(src));
  assert(data.fun_app_anon.fun->tag == EXP_ID);
  assert(sb_count(data.fun_app_anon.args) == 0);
  free_inner_exp(data);

  src = "a(@42)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_FUN_APP_ANON);
  assert(data.len == strlen(src));
  assert(data.fun_app_anon.fun->tag == EXP_ID);
  assert(sb_count(data.fun_app_anon.args) == 1);
  free_inner_exp(data);

  src = "a(b, @42)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_FUN_APP_ANON);
  assert(data.len == strlen(src));
  assert(data.fun_app_anon.fun->tag == EXP_ID);
  assert(sb_count(data.fun_app_anon.args) == 2);
  free_inner_exp(data);

  src = "a(b = @42)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_FUN_APP_NAMED);
  assert(data.len == strlen(src));
  assert(data.fun_app_named.fun->tag == EXP_ID);
  assert(sb_count(data.fun_app_named.args) == 1);
  assert(sb_count(data.fun_app_named.sids) == 1);
  free_inner_exp(data);

  src = "a(b = c, d = e)";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_FUN_APP_NAMED);
  assert(data.len == strlen(src));
  assert(data.fun_app_named.fun->tag == EXP_ID);
  assert(sb_count(data.fun_app_named.args) == 2);
  assert(sb_count(data.fun_app_named.sids) == 2);
  free_inner_exp(data);

  src = "(@a) as @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_CAST);
  assert(data.len == strlen(src));
  assert(data.cast.inner->tag == EXP_PRODUCT_ANON);
  assert(data.cast.type->tag == TYPE_PTR);
  free_inner_exp(data);

  src = "(@a) + @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BIN_OP);
  assert(data.len == strlen(src));
  assert(data.bin_op.op == OP_PLUS);
  assert(data.bin_op.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.bin_op.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@a) < @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BIN_OP);
  assert(data.len == strlen(src));
  assert(data.bin_op.op == OP_LT);
  assert(data.bin_op.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.bin_op.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@a) << @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_BIN_OP);
  assert(data.len == strlen(src));
  assert(data.bin_op.op == OP_SHIFT_L);
  assert(data.bin_op.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.bin_op.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@a) += @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ASSIGN);
  assert(data.len == strlen(src));
  assert(data.assign.op == ASSIGN_PLUS);
  assert(data.assign.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.assign.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@a) <<= @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ASSIGN);
  assert(data.len == strlen(src));
  assert(data.assign.op == ASSIGN_SHIFT_L);
  assert(data.assign.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.assign.rhs->tag == EXP_REF);
  free_inner_exp(data);

  src = "(@a) = @b";
  assert(parse_exp(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == EXP_ASSIGN);
  assert(data.len == strlen(src));
  assert(data.assign.op == ASSIGN_REGULAR);
  assert(data.assign.lhs->tag == EXP_PRODUCT_ANON);
  assert(data.assign.rhs->tag == EXP_REF);
  free_inner_exp(data);
}

void test_meta(void) {
  char *src = "  foo";
  OoError err;
  AsgMeta data;

  assert(parse_meta(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == META_NULLARY);
  assert(data.len == strlen(src));
  assert(data.name_len == 3);
  assert(strncmp(data.name, "foo", 3) == 0);
  free_inner_meta(data);

  src = "foo = 42";
  assert(parse_meta(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == META_UNARY);
  assert(data.len == strlen(src));
  assert(data.name_len == 3);
  assert(strncmp(data.name, "foo", 3) == 0);
  assert(data.unary.len == 2);
  assert(data.unary.tag == LITERAL_INT);
  free_inner_meta(data);

  src = "foo(bar)";
  assert(parse_meta(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == META_NESTED);
  assert(data.len == strlen(src));
  assert(data.name_len == 3);
  assert(strncmp(data.name, "foo", 3) == 0);
  assert(sb_count(data.nested) == 1);
  free_inner_meta(data);

  src = "foo(bar, baz = 42)";
  assert(parse_meta(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == META_NESTED);
  assert(data.len == strlen(src));
  assert(data.name_len == 3);
  assert(strncmp(data.name, "foo", 3) == 0);
  assert(sb_count(data.nested) == 2);
  free_inner_meta(data);
}

void test_use_tree(void) {
  char *src = "a";
  OoError err;
  AsgUseTree data;

  assert(parse_use_tree(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == USE_TREE_LEAF);
  assert(data.sid.len == 1);
  free_inner_use_tree(data);

  src = "a as b";
  assert(parse_use_tree(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == USE_TREE_RENAME);
  assert(data.sid.len == 1);
  assert(data.rename.len == 1);
  free_inner_use_tree(data);

  src = "a::b::c";
  assert(parse_use_tree(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == USE_TREE_BRANCH);
  assert(data.sid.len == 1);
  assert(sb_count(data.branch) == 1);
  assert(data.branch[0].tag == USE_TREE_BRANCH);
  assert(sb_count(data.branch[0].branch) == 1);
  assert(data.branch[0].branch[0].tag == USE_TREE_LEAF);
  free_inner_use_tree(data);

  src = "a::{a}";
  assert(parse_use_tree(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == USE_TREE_BRANCH);
  assert(data.sid.len == 1);
  assert(sb_count(data.branch) == 1);
  assert(data.branch[0].tag == USE_TREE_LEAF);
  free_inner_use_tree(data);

  src = "super::{dep, magic, mod}";
  assert(parse_use_tree(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == USE_TREE_BRANCH);
  assert(data.sid.len == 5);
  assert(sb_count(data.branch) == 3);
  assert(data.branch[0].tag == USE_TREE_LEAF);
  assert(data.branch[1].tag == USE_TREE_LEAF);
  assert(data.branch[2].tag == USE_TREE_LEAF);
  free_inner_use_tree(data);
}

void test_item(void) {
  char *src = "pub use a::b";
  OoError err;
  AsgItem data;

  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_USE);
  assert(data.pub);
  assert(data.use.tag == USE_TREE_BRANCH);
  free_inner_item(data);

  src = "use a::b";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_USE);
  assert(!data.pub);
  assert(data.use.tag == USE_TREE_BRANCH);
  free_inner_item(data);

  src = "type a =@ b";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_TYPE);
  assert(!data.pub);
  assert(data.type.sid.len == 1);
  assert(data.type.type.tag == TYPE_PTR);
  free_inner_item(data);

  src = "val a = @b";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_VAL);
  assert(!data.pub);
  assert(!data.val.mut);
  assert(data.val.sid.len == 1);
  assert(data.val.exp.tag == EXP_REF);
  free_inner_item(data);

  src = "pub val mut a = @b";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_VAL);
  assert(data.pub);
  assert(data.val.mut);
  assert(data.val.sid.len == 1);
  assert(data.val.exp.tag == EXP_REF);
  free_inner_item(data);

  src = "fn a = () {}";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FUN);
  assert(!data.pub);
  assert(data.fun.sid.len == 1);
  assert(sb_count(data.fun.type_args) == 0);
  assert(sb_count(data.fun.arg_sids) == 0);
  assert(sb_count(data.fun.arg_types) == 0);
  assert(data.fun.ret.tag = TYPE_PRODUCT_ANON);
  assert(sb_count(data.fun.ret.product_anon) == 0);
  assert(sb_count(data.fun.body.exps) == 0);
  free_inner_item(data);

  src = "fn a = <T> => (b: T) -> @c {a}";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FUN);
  assert(!data.pub);
  assert(data.fun.sid.len == 1);
  assert(sb_count(data.fun.type_args) == 1);
  assert(sb_count(data.fun.arg_sids) == 1);
  assert(sb_count(data.fun.arg_types) == 1);
  assert(data.fun.ret.tag = TYPE_PTR);
  assert(sb_count(data.fun.body.exps) == 1);
  free_inner_item(data);

  src = "fn a = <T, U> => (b: T, d: U) -> @c {a; b}";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FUN);
  assert(!data.pub);
  assert(data.fun.sid.len == 1);
  assert(sb_count(data.fun.type_args) == 2);
  assert(sb_count(data.fun.arg_sids) == 2);
  assert(sb_count(data.fun.arg_types) == 2);
  assert(data.fun.ret.tag = TYPE_PTR);
  assert(sb_count(data.fun.body.exps) == 2);
  free_inner_item(data);

  src = "ffi use(foo.h)";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FFI_INCLUDE);
  assert(!data.pub);
  assert(data.ffi_include.include == src + 8);
  assert(data.ffi_include.include_len == 5);
  free_inner_item(data);

  src = "ffi a: @B";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FFI_VAL);
  assert(!data.pub);
  assert(!data.ffi_val.mut);
  assert(data.ffi_val.sid.len == 1);
  assert(data.ffi_val.type.tag == TYPE_PTR);
  free_inner_item(data);

  src = "pub ffi mut a: @B";
  assert(parse_item(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(data.tag == ITEM_FFI_VAL);
  assert(data.pub);
  assert(data.ffi_val.mut);
  assert(data.ffi_val.sid.len == 1);
  assert(data.ffi_val.type.tag == TYPE_PTR);
  free_inner_item(data);
}

void test_file(void) {
  char *src = "type a = b";
  OoError err;
  AsgFile data;

  assert(parse_file(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(sb_count(data.items) == 1);
  assert(sb_count(data.attrs) == 1);
  assert(data.items[0].tag == ITEM_TYPE);
  assert(!data.items[0].pub);
  free_inner_file(data);

  src = "#[foo]#[bar] type a = b";
  assert(parse_file(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(sb_count(data.items) == 1);
  assert(sb_count(data.attrs) == 1);
  assert(data.items[0].tag == ITEM_TYPE);
  assert(!data.items[0].pub);
  assert(sb_count(data.attrs[0]) == 2);
  free_inner_file(data);

  src = "type a = b type c = @d";
  assert(parse_file(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(sb_count(data.items) == 2);
  assert(sb_count(data.attrs) == 2);
  assert(data.items[0].tag == ITEM_TYPE);
  assert(!data.items[0].pub);
  assert(data.items[1].tag == ITEM_TYPE);
  assert(!data.items[1].pub);
  free_inner_file(data);
}

int main(void) {
  test_sid();
  test_id();
  test_macro_inv();
  test_literal();
  test_repeat();
  test_type();
  test_pattern();
  test_exp();
  test_meta();
  test_use_tree();
  test_item();
  test_file();

  return 0;
}
