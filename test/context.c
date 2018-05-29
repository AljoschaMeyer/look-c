#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../src/stretchy_buffer.h"
#include "../src/context.h"
#include "../src/util.h"

// void test_file_not_found(void) {
//   OoError err;
//
//   OoContext cx;
//   cx.deps = "foo";
//   cx.deps_mod = NULL;
//   cx.mods = "bar";
//   cx.mods_mod = NULL;
//   cx.features = raxNew();
//   cx.files = raxNew();
//
//   assert(oo_get_file(&cx, &err, str_new("baz", 3)) == NULL);
//   assert(err.tag == OO_ERR_FILE);
//
//   oo_context_free(&cx);
// }
//
// void test_read_file(void) {
//   char path[PATH_MAX];
//   getcwd(path, sizeof(path));
//   strcat(path, "/test/option.oo");
//
//   OoError err;
//
//   OoContext cx;
//   cx.deps = "foo";
//   cx.deps_mod = NULL;
//   cx.mods = "bar";
//   cx.mods_mod = NULL;
//   cx.features = raxNew();
//   cx.files = raxNew();
//
//   AsgFile *asg = oo_get_file(&cx, &err, str_new(path, strlen(path)));
//   assert(asg != NULL);
//   assert(err.tag == OO_ERR_NONE);
//
//   oo_context_free(&cx);
//   free_inner_file(*asg);
//   free((void *) asg->str.start);
//   free(asg);
// }
//
// void test_get_file_ids(void) {
//   char deps[PATH_MAX];
//   getcwd(deps, sizeof(deps));
//   strcat(deps, "/test/deps");
//   char mods[PATH_MAX];
//   getcwd(mods, sizeof(mods));
//   strcat(mods, "/test");
//
//   Str *ids = NULL;
//   sb_push(ids, str_new("mod", 3));
//   sb_push(ids, str_new("foo", 3));
//
//   OoError err;
//
//   OoContext cx;
//   cx.deps = deps;
//   cx.deps_mod = NULL;
//   cx.mods = mods;
//   cx.mods_mod = NULL;
//   cx.features = raxNew();
//   cx.files = raxNew();
//
//   // foo.oo
//
//   AsgFile *asg = oo_get_file_ids(&cx, &err, ids);
//   assert(asg != NULL);
//   assert(err.tag == OO_ERR_NONE);
//   asg = oo_get_file_ids(&cx, &err, ids);
//   assert(asg != NULL);
//   assert(err.tag == OO_ERR_NONE);
//
//   oo_context_free(&cx);
//   cx.features = raxNew();
//   cx.files = raxNew();
//   free_inner_file(*asg);
//   free((void *) asg->str.start);
//   free(asg);
//   sb_free(ids);
//
//   // dir/bar.oo
//
//   ids = NULL;
//   sb_push(ids, str_new("mod", 3));
//   sb_push(ids, str_new("dir", 3));
//   sb_push(ids, str_new("bar", 3));
//
//   asg = oo_get_file_ids(&cx, &err, ids);
//   assert(asg != NULL);
//   assert(err.tag == OO_ERR_NONE);
//
//   oo_context_free(&cx);
//   cx.features = raxNew();
//   cx.files = raxNew();
//   free_inner_file(*asg);
//   free((void *) asg->str.start);
//   free(asg);
//   sb_free(ids);
//
//   // deps/foo/lib.oo
//
//   ids = NULL;
//   sb_push(ids, str_new("dep", 3));
//   sb_push(ids, str_new("foo", 3));
//
//   asg = oo_get_file_ids(&cx, &err, ids);
//   assert(asg != NULL);
//   assert(err.tag == OO_ERR_NONE);
//
//   oo_context_free(&cx);
//   free_inner_file(*asg);
//   free((void *) asg->str.start);
//   free(asg);
//   sb_free(ids);
// }

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

    // TODO test builtin namespaces: bool::and, u16::as_u32, etc.
    // TODO test sum types

    raxFree(features);
    oo_cx_free(&cx);
}

int main(void) {
  // test_file_not_found();
  // test_read_file();
  // test_get_file_ids();

  test_parsing();
  test_duplicates();
  test_use_duplicates();

  return 0;
}
