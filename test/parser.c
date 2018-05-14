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

  return 0;
}
