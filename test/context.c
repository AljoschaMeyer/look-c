#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "../src/stretchy_buffer.h"
#include "../src/context.h"

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

  AsgFile *asg = oo_get_file(&cx, &err, path, strlen(path));
  assert(asg != NULL);
  assert(err.tag == OO_ERR_NONE);

  oo_context_free(&cx);
  free_inner_file(*asg);
  free((void *) asg->str.start);
  free(asg);
}

void test_file_not_found(void) {
  OoError err;

  OoContext cx;
  cx.deps = "foo";
  cx.mods = "bar";
  cx.features = raxNew();
  cx.files = raxNew();

  assert(oo_get_file(&cx, &err, "baz", 3) == NULL);
  assert(err.tag == OO_ERR_FILE);

  oo_context_free(&cx);
}

int main(void) {
  test_file_not_found();
  test_read_file();

  return 0;
}
