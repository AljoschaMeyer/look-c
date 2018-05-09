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

  src = "(a = A)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.types) == 1);
  assert(data.product_named.types[0].tag == TYPE_ID);
  assert(data.product_named.types[0].src == src + 5);
  assert(data.product_named.types[0].len == 2);
  assert(sb_count(data.product_named.sids) == 1);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  free_inner_type(data);
  
  src = "(a = A, b = @B)";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_PRODUCT_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.product_named.types) == 2);
  assert(data.product_named.types[0].tag == TYPE_ID);
  assert(data.product_named.types[0].src == src + 5);
  assert(data.product_named.types[0].len == 2);
  assert(data.product_named.types[1].tag == TYPE_PTR);
  assert(data.product_named.types[1].src == src + 12);
  assert(data.product_named.types[1].len == 3);
  assert(sb_count(data.product_named.sids) == 2);
  assert(data.product_named.sids[0].src == src + 1);
  assert(data.product_named.sids[0].len == 1);
  assert(data.product_named.sids[1].src == src + 8);
  assert(data.product_named.sids[1].len == 1);
  free_inner_type(data);

  src = "(a = @A) -> @A";
  assert(parse_type(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.tag == TYPE_FUN_NAMED);
  assert(data.len == strlen(src));
  assert(sb_count(data.fun_named.arg_types) == 1);
  assert(data.fun_named.arg_types[0].tag == TYPE_PTR);
  assert(data.fun_named.arg_types[0].src == src + 5);
  assert(data.fun_named.arg_types[0].len == 3);
  assert(data.fun_named.ret->tag == TYPE_PTR);
  assert(data.fun_named.ret->len == 3);
  free_inner_type(data);
}

int main(void)
{
  test_sid();
  test_id();
  test_macro_inv();
  test_literal();
  test_repeat();
  test_type();

  return 0;
}


