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

  src = "abc::def::ghi";
  l = parse_id(src, &err, &data);
  assert(l == 13);
  assert(err.tag == ERR_NONE);
  assert(data.sids_len == 3);
  assert(data.sids[0].src == src);
  assert(data.sids[1].src == src + 5);
  assert(data.sids[2].src == src + 10);
  free_inner_id(data);
}

int main(void)
{
  test_sid();
  test_id();

  return 0;
}


