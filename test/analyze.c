#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../src/stretchy_buffer.h"
#include "../src/context.h"

void test_init_item_maps(void) {
  char deps[PATH_MAX];
  getcwd(deps, sizeof(deps));
  strcat(deps, "/test/deps");
  char mods[PATH_MAX];
  getcwd(mods, sizeof(mods));
  strcat(mods, "/test");

  Str *ids = NULL;
  sb_push(ids, str_new("mod", 3));
  sb_push(ids, str_new("foo", 3));

  Str *bar_ids = NULL;
  sb_push(bar_ids, str_new("mod", 3));
  sb_push(bar_ids, str_new("dir", 3));
  sb_push(bar_ids, str_new("bar", 3));

  Str *baz_ids = NULL;
  sb_push(baz_ids, str_new("mod", 3));
  sb_push(baz_ids, str_new("baz", 3));

  OoError err;

  OoContext cx;
  cx.deps = deps;
  cx.mods = mods;
  cx.features = raxNew();
  cx.files = raxNew();

  AsgFile *asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);
  asg = oo_get_file_ids(&cx, &err, ids); // test that this is idempotent
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  AsgFile *bar = oo_get_file_ids(&cx, &err, bar_ids);
  assert(bar != NULL);
  AsgFile *baz = oo_get_file_ids(&cx, &err, baz_ids);
  assert(baz != NULL);

  AsgBinding b;

  oo_init_item_maps(asg, &cx, &err);
  assert(err.tag == OO_ERR_NONE);
  assert(asg->did_init_bindings_by_sid);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "a", 1);
  assert(b.tag == BINDING_TYPE);
  assert(b.type == &asg->items[0]);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "c", 1);
  assert(b.tag == BINDING_VAL);
  assert(b.val == &asg->items[1]);

  b = *(AsgBinding *) raxFind(asg->pub_bindings_by_sid, "a", 1);
  assert(b.tag == BINDING_TYPE);
  assert(b.type == &asg->items[0]);

  assert(raxFind(asg->pub_bindings_by_sid, "c", 1) == raxNotFound);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "e", 1);
  assert(b.tag == BINDING_VAL);
  assert(b.val == &bar->items[0]);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "qux", 3);
  assert(b.tag == BINDING_MOD);
  assert(b.mod == baz);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "f", 1);
  assert(b.tag == BINDING_TYPE);
  assert(b.type == &baz->items[0]);

  b = *(AsgBinding *) raxFind(asg->bindings_by_sid, "g", 1);
  assert(b.tag == BINDING_TYPE);
  assert(b.type == &baz->items[0]);

  // TODO test more stuff: using sum variants

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);
  sb_free(bar_ids);
  sb_free(baz_ids);
}

void test_init_item_maps_duplicates(void) {
  char deps[PATH_MAX];
  getcwd(deps, sizeof(deps));
  strcat(deps, "/test/deps");
  char mods[PATH_MAX];
  getcwd(mods, sizeof(mods));
  strcat(mods, "/test");

  Str *ids = NULL;
  sb_push(ids, str_new("mod", 3));
  sb_push(ids, str_new("duplicates", 10));

  OoError err;

  OoContext cx;
  cx.deps = deps;
  cx.mods = mods;
  cx.features = raxNew();
  cx.files = raxNew();

  AsgFile *asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);
  asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_init_item_maps(asg, &cx, &err);
  assert(err.tag == OO_ERR_DUP_ID);
  assert(str_eq_parts(err.dup, "a", 1));

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);
}

void test_init_item_maps_use_duplicates(void) {
  char deps[PATH_MAX];
  getcwd(deps, sizeof(deps));
  strcat(deps, "/test/deps");
  char mods[PATH_MAX];
  getcwd(mods, sizeof(mods));
  strcat(mods, "/test");

  Str *ids = NULL;
  sb_push(ids, str_new("mod", 3));
  sb_push(ids, str_new("use_duplicates", 14));

  OoError err;

  OoContext cx;
  cx.deps = deps;
  cx.mods = mods;
  cx.features = raxNew();
  cx.files = raxNew();

  AsgFile *asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);
  asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_init_item_maps(asg, &cx, &err);
  assert(err.tag == OO_ERR_DUP_ID);
  assert(str_eq_parts(err.dup, "a", 1));

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);
}

int main(void) {
  test_init_item_maps();
  test_init_item_maps_duplicates();
  test_init_item_maps_use_duplicates();

  return 0;
}
