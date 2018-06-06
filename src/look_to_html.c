#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "context.h"
#include "stretchy_buffer.h"

// output dir
// assign url to each file
// assign id to each binding
// write files with spans for syntax highlighting and ids linking to their binding
// write files for directories?

void render_file(AsgFile *asg, FILE *f);

// Determine where to render the AsgFile of the given path, and open that file.
FILE  *open_file(OoContext *cx, const char *src_path, const char *out_dir) {
  char path[PATH_MAX];
  size_t out_dir_len = strlen(out_dir);
  size_t src_len = strlen(src_path);
  size_t path_len = out_dir_len;

  memcpy(path, out_dir, out_dir_len);

  size_t deps_len = strlen(cx->deps);
  if (strncmp(cx->deps, src_path, deps_len) == 0) { // src_path is in deps
    memcpy(path + path_len, "/dep/", 5);
    path_len += 5;
    memcpy(path + path_len, src_path + deps_len, src_len - deps_len);
    path_len += src_len - deps_len;
  } else { // src_path is in mod
    size_t mods_len = strlen(cx->mods);
    memcpy(path + path_len, "/", 1);
    path_len += 1;
    memcpy(path + path_len, src_path + mods_len, src_len - mods_len);
    path_len += src_len - mods_len;
  }

  memcpy(path + path_len, ".html", 6); // 6 to add the 0 byte
  path_len += 6;

  return fopen(path, "w+");
}

// Writes html files to the given directory, with syntax-highlighted source and
// correctly hyperlinked bindings.
// This may have weird output for code that is skipped during conditional compilation.
//
// oo_cx_fine_bindings must have been called on the given context.
// outdir must exist and be writable.
//
// Returns false if an error occured.
bool look_to_html(OoContext *cx, const char *out_dir) {
  for (size_t i = 0; i < (size_t) sb_count(cx->files); i++) {
    FILE *f = open_file(cx, cx->files[i]->path, out_dir);
    if (f == NULL) {
      return false;
    }

    render_file(cx->files[i], f);
    fclose(f);
  }

  return true;
}

const char *render_item(AsgItem *item, FILE *f) {
  fprintf(f, "<span class=\"item ");
  switch (item->tag) {
    case ITEM_USE:
      fprintf(f, "item_use");
      break;
    case ITEM_TYPE:
      fprintf(f, "item_type");
      break;
    case ITEM_VAL:
      fprintf(f, "item_val");
      break;
    case ITEM_FUN:
      fprintf(f, "item_fun");
      break;
    case ITEM_FFI_INCLUDE:
      fprintf(f, "item_ffi_include");
      break;
    case ITEM_FFI_VAL:
      fprintf(f, "item_ffi_val");
      break;
  }
  fprintf(f, "\">");

  const char *src = item->str.start;
  while (*src == ' ' || *src == '\n') {
    fprintf(f, "%c", *src);
    src += 1;
  }

  switch (item->tag) {
    case ITEM_USE:
      fprintf(f, "<span class=\"kw kw_use\">use</span>");
      src += 3;
      break;
    case ITEM_TYPE:
      fprintf(f, "<span class=\"kw kw_type\">type</span>");
      src += 4;
      break;
    case ITEM_VAL:
      fprintf(f, "<span class=\"kw kw_val\">val</span>");
      src += 3;
      break;
    case ITEM_FUN:
      fprintf(f, "<span class=\"kw kw_fn\">fn</span>");
      src += 2;
      break;
    case ITEM_FFI_INCLUDE:
      fprintf(f, "item_ffi_include"); // TODO
      break;
    case ITEM_FFI_VAL:
      fprintf(f, "item_ffi_val"); // TODO
      break;
  }



  fprintf(f, "%s", "render_item"); // TODO

  fprintf(f, "</span>");
  return item->str.start + item->str.len;
}

void render_file(AsgFile *asg, FILE *f) {
  const char *src = asg->str.start;
  size_t i = 0;

  while (src < asg->str.start + asg->str.len) {
    if (src < asg->items[i].str.start) {
      fprintf(f, "%c", *src);
      src += 1;
    } else {
      src = render_item(&asg->items[i], f);
      i += 1;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("%s\n", "Must supply the directory to render.");
    return 1;
  }
  size_t dir_len = strlen(argv[1]);

  char *out_dir = malloc(dir_len + 6);
  memcpy(out_dir, argv[1], dir_len);
  memcpy(out_dir + dir_len, "/html", 6);

  char *deps_dir = malloc(dir_len + 9);
  memcpy(deps_dir, argv[1], dir_len);
  memcpy(deps_dir + dir_len, "/devdeps", 9);

  char *mod_dir = malloc(dir_len + 5);
  memcpy(mod_dir, argv[1], dir_len);
  memcpy(mod_dir + dir_len, "/src", 5);

  OoContext cx;
  OoError err;
  err.tag = OO_ERR_NONE;
  rax *features = raxNew();
  raxInsert(features, "dev", 3, NULL, NULL);
  raxInsert(features, "debug", 5, NULL, NULL);
  raxInsert(features, "linux", 5, NULL, NULL);
  raxInsert(features, "posix", 5, NULL, NULL);

  oo_cx_init(&cx, mod_dir, deps_dir);

  oo_cx_parse(&cx, &err, features);
  if (err.tag != OO_ERR_NONE) {
    err_print(&err);
    return (int) err.tag;
  }

  oo_cx_coarse_bindings(&cx, &err);
  if (err.tag != OO_ERR_NONE) {
    err_print(&err);
    return (int) err.tag;
  }

  oo_cx_fine_bindings(&cx, &err);
  if (err.tag != OO_ERR_NONE) {
    err_print(&err);
    return (int) err.tag;
  }

  int exit = 0;

  mkdir(out_dir, 0700);

  if (!look_to_html(&cx, out_dir)) {
    printf("%s\n", "Failed to write html.");
    exit = 1;
  }

  raxFree(features);
  oo_cx_free(&cx);
  free(out_dir);
  free(deps_dir);
  free(mod_dir);

  return exit;
}
