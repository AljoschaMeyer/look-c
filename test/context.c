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

void test_coarse_bindings(void) {
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

    raxFree(features);
    oo_cx_free(&cx);
}

void test_fine_bindings(void) {
    char mods[PATH_MAX];
    getcwd(mods, sizeof(mods));
    strcat(mods, "/test/example_bindings");
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

    oo_cx_coarse_bindings(&cx, &err);
    assert(err.tag == OO_ERR_NONE);

    oo_cx_fine_bindings(&cx, &err);
    assert(err.tag == OO_ERR_NONE);

    char lib[PATH_MAX];
    getcwd(lib, sizeof(lib));
    strcat(lib, "/test/example_bindings/lib.oo");
    assert(strcmp(cx.files[0]->path, lib) == 0);

    assert(cx.files[0]->items[0].type.type.generic.inner->product_anon[0].id.sids[0].binding.tag == BINDING_TYPE_VAR);
    assert(
      cx.files[0]->items[0].type.type.generic.inner->product_anon[0].id.sids[0].binding.type_var ==
      &cx.files[0]->items[0].type.type.generic.args[0]
    );

    assert(cx.files[0]->items[0].type.type.generic.inner->product_anon[1].id.sids[0].binding.tag == BINDING_TYPE);
    assert(
      cx.files[0]->items[0].type.type.generic.inner->product_anon[1].id.sids[0].binding.type ==
      &cx.files[0]->items[2].type
    );

    assert(cx.files[0]->items[0].type.type.generic.inner->product_anon[2].id.sids[0].binding.tag == BINDING_PRIMITIVE);

    oo_cx_kind_checking(&cx, &err);
    assert(err.tag == OO_ERR_NONE);

    raxFree(features);
    oo_cx_free(&cx);
}

int main(void) {
  test_coarse_bindings();
  test_duplicates();
  test_use_duplicates();
  test_fine_bindings();

  return 0;
}
