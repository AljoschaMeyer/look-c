#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/stretchy_buffer.h"
#include "../src/parser.h"

void test_cc(void) {
  char *src = "#[cc = \"foo\"]type a = b #[cc = \"bar\"]fn c = () {#[cc = \"bar\"]a; #[cc = \"baz\"]b}";
  ParserError err;
  AsgFile data;

  assert(parse_file(src, &err, &data) == strlen(src));
  assert(err.tag == ERR_NONE);
  assert(data.len == strlen(src));
  assert(sb_count(data.items) == 2);
  assert(sb_count(data.attrs) == 2);
  assert(data.items[0].tag == ITEM_TYPE);
  assert(sb_count(data.attrs[0]) == 1);
  assert(data.items[1].tag == ITEM_FUN);
  assert(sb_count(data.attrs[1]) == 1);
  assert(sb_count(data.items[1].fun.body.attrs) == 2);
  assert(sb_count(data.items[1].fun.body.attrs[0]) == 1);
  assert(sb_count(data.items[1].fun.body.attrs[1]) == 1);

  rax *features = raxNew();
  raxInsert(features, "bar", 3, NULL, NULL);
  oo_filter_cc(&data, features);

  assert(data.len == strlen(src));
  assert(sb_count(data.items) == 1);
  assert(sb_count(data.attrs) == 1);
  assert(data.items[0].tag == ITEM_FUN);
  assert(sb_count(data.attrs[0]) == 1);
  assert(sb_count(data.items[0].fun.body.attrs) == 1);
  assert(sb_count(data.items[0].fun.body.attrs[0]) == 1);

  free_inner_file(data);
  raxFree(features);
}

int main(void) {
  test_cc();

  return 0;
}
