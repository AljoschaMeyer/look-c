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
  assert(err.tag == OO_ERR_NONE);
  assert(asg->did_init_items_by_sid);
  assert(raxFind(asg->items_by_sid, "a", 1) != raxNotFound);
  assert(raxFind(asg->items_by_sid, "a", 1) == &asg->items[0]);
  assert(raxFind(asg->items_by_sid, "c", 1) == &asg->items[1]);
  assert(raxFind(asg->pub_items_by_sid, "a", 1) == &asg->items[0]);
  assert(raxFind(asg->pub_items_by_sid, "c", 1) == raxNotFound);

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);
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

int main(void) {
  test_init_item_maps();
  test_init_item_maps_duplicates();

  return 0;
}
