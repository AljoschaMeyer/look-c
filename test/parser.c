#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

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
  assert(data.sids_len == 1);
  assert(data.sids[0].src == src);
  free_inner_id(data);

  src = "abc:: def ::ghi";
  l = parse_id(src, &err, &data);
  assert(l == 15);
  assert(err.tag == ERR_NONE);
  assert(data.sids_len == 3);
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

  assert(parse_repeat(src, &err, &data) == 11);
  assert(err.tag == ERR_NONE);
  assert(data.tag == REPEAT_BIN_OP);
  assert(data.bin_op.lhs->tag == REPEAT_INT);
  assert(data.bin_op.lhs->len == 2);
  assert(data.bin_op.rhs->tag == REPEAT_MACRO);
  assert(data.bin_op.rhs->macro.name == src + 6);
  assert(data.bin_op.rhs->macro.name_len == 3);

  free_inner_repeat(data);
}

int main(void)
{
  test_sid();
  test_id();
  test_macro_inv();
  test_literal();
  test_repeat();

  return 0;
}


