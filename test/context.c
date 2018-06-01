#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../src/stretchy_buffer.h"
#include "../src/context.h"
#include "../src/util.h"

void test_duplicates(void) {
  char mods[PATH_MAX];
  getcwd(mods, sizeof(mods));
  strcat(mods, "/test/example_duplicates");
  char deps[PATH_MAX];
  getcwd(deps, sizeof(deps));
  strcat(deps, "/test/example_deps");

  rax *features = raxNew();
  OoError err;
  err.tag = OO_ERR_NONE;
  OoContext cx;
  oo_cx_init(&cx, mods, deps);

  oo_cx_parse(&cx, &err, features);
  assert(err.tag == OO_ERR_NONE);
  assert(sb_count(cx.sources) == 2);

  oo_cx_coarse_bindings(&cx, &err);
  assert(err.tag == OO_ERR_DUP_ID_ITEM);

  raxFree(features);
  oo_cx_free(&cx);
}

void test_use_duplicates(void) {
  char mods[PATH_MAX];
  getcwd(mods, sizeof(mods));
  strcat(mods, "/test/example_use_duplicates");
  char deps[PATH_MAX];
  getcwd(deps, sizeof(deps));
  strcat(deps, "/test/example_deps");

  rax *features = raxNew();
  OoError err;
  err.tag = OO_ERR_NONE;
  OoContext cx;
  oo_cx_init(&cx, mods, deps);

  oo_cx_parse(&cx, &err, features);
  assert(err.tag == OO_ERR_NONE);
  assert(sb_count(cx.sources) == 3);

  oo_cx_coarse_bindings(&cx, &err);
  assert(err.tag == OO_ERR_DUP_ID_ITEM_USE);

  raxFree(features);
  oo_cx_free(&cx);
}

void test_parsing(void) {
    char mods[PATH_MAX];
    getcwd(mods, sizeof(mods));
    strcat(mods, "/test/example_code");
    char deps[PATH_MAX];
    getcwd(deps, sizeof(deps));
    strcat(deps, "/test/example_deps");

    rax *features = raxNew();
    OoError err;
    err.tag = OO_ERR_NONE;
    OoContext cx;
    oo_cx_init(&cx, mods, deps);

    oo_cx_parse(&cx, &err, features);
    assert(err.tag == OO_ERR_NONE);
    assert(sb_count(cx.sources) == 5);

    oo_cx_coarse_bindings(&cx, &err);
    assert(err.tag == OO_ERR_NONE);
    // err_print(&err);

    // TODO test builtin namespaces: bool::and, u16::as_u32, etc.

    raxFree(features);
    oo_cx_free(&cx);
}

int main(void) {
  test_parsing();
  test_duplicates();
  test_use_duplicates();

  return 0;
}
