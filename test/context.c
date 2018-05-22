#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../src/stretchy_buffer.h"
#include "../src/context.h"

void test_file_not_found(void) {
  OoError err;

  OoContext cx;
  cx.deps = "foo";
  cx.mods = "bar";
  cx.features = raxNew();
  cx.files = raxNew();

  assert(oo_get_file(&cx, &err, str_new("baz", 3)) == NULL);
  assert(err.tag == OO_ERR_FILE);

  oo_context_free(&cx);
}

void test_read_file(void) {
  char path[PATH_MAX];
  getcwd(path, sizeof(path));
  strcat(path, "/test/option.oo");

  OoError err;

  OoContext cx;
  cx.deps = "foo";
  cx.mods = "bar";
  cx.features = raxNew();
  cx.files = raxNew();

  AsgFile *asg = oo_get_file(&cx, &err, str_new(path, strlen(path)));
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
}

void test_get_file_ids(void) {
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

  // foo.oo

  AsgFile *asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);
  asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_context_free(&cx);
  cx.features = raxNew();
  cx.files = raxNew();
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);

  // dir/bar.oo

  ids = NULL;
  sb_push(ids, str_new("mod", 3));
  sb_push(ids, str_new("dir", 3));
  sb_push(ids, str_new("bar", 3));

  asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_context_free(&cx);
  cx.features = raxNew();
  cx.files = raxNew();
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);

  // deps/foo/lib.oo

  ids = NULL;
  sb_push(ids, str_new("dep", 3));
  sb_push(ids, str_new("foo", 3));

  asg = oo_get_file_ids(&cx, &err, ids);
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
  sb_free(ids);
}

int main(void) {
  test_file_not_found();
  test_read_file();
  test_get_file_ids();

  return 0;
}
