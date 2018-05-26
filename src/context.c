#include <stdio.h>
#include <stdlib.h>

#include "context.h"
#include "parser.h"
#include "rax.h"
#include "cc.h"
#include "stretchy_buffer.h"

AsgFile *oo_get_file(OoContext *cx, OoError *err, Str path) {
  char *src = NULL;

  void *f = raxFind(cx->files, path.start, path.len);
  if (f == raxNotFound) {
    FILE *f = fopen(path.start, "r");
    if (f == NULL) {
      goto file_err;
    }
    if (fseek(f, 0, SEEK_END)) {
      goto file_err;
    }
    long fsize = ftell(f);
    if (fsize < 0) {
      goto file_err;
    }
    rewind(f);

    src = malloc(fsize + 1);
    fread(src, fsize, 1, f);
    if (ferror(f)) {
      goto file_err;
    }
    src[fsize] = 0;

    if (fclose(f)) {
      goto file_err;
    }
    // end file handling

    AsgFile *asg = malloc(sizeof(AsgFile));
    parse_file(src, &err->parser, asg);
    if (err->parser.tag != ERR_NONE) {
      free(src);
      free(asg);
      err->tag = OO_ERR_SYNTAX;
      return NULL;
    }

    oo_filter_cc(asg, cx->features);

    raxInsert(cx->files, path.start, path.len, asg, NULL);

    err->tag = OO_ERR_NONE;
    return asg;
  } else {
    err->tag = OO_ERR_NONE;
    return (AsgFile *) f;
  }

  file_err:
  free(src);
  err->tag = OO_ERR_FILE;
  return NULL;
}

AsgFile *oo_get_file_ids(OoContext *cx, OoError *err, Str *ids /* stretchy buffer */) {
  if (sb_count(ids) < 2) {
    err->tag = OO_ERR_IMPORT;
    return NULL;
  }
  if (str_eq_parts(ids[0], "mod", 3)) {
    size_t mods_len = strlen(cx->mods);
    Str s;
    s.len = mods_len;
    size_t count = (size_t) sb_count(ids);
    for (size_t i = 1; i < count; i++) {
      s.len += 1; // "/"
      s.len += ids[i].len;
    }
    s.len += 4; // ".oo" NULL

    char *path = malloc(s.len);
    strcpy(path, cx->mods);
    size_t offset = mods_len;
    for (size_t i = 1; i < count; i++) {
      path[offset] = '/';
      offset += 1;
      strncpy(path + offset, ids[i].start, ids[i].len);
      offset += ids[i].len;
    }
    memcpy(path + offset, ".oo", 4);
    s.start = path;

    AsgFile *asg = oo_get_file(cx, err, s);
    free(path);
    return asg;
  } else if (str_eq_parts(ids[0], "dep", 3)) {
    size_t deps_len = strlen(cx->deps);
    Str s;
    s.len = deps_len + 1 + ids[1].len + 8 /* "/lib.oo" NULL */;

    char *path = malloc(s.len);
    strcpy(path, cx->deps);
    path[deps_len] = '/';
    strncpy(path + deps_len + 1, ids[1].start, ids[1].len);
    strncpy(path + deps_len + 1 + ids[1].len, "/lib.oo", 8);
    s.start = path;

    AsgFile *asg = oo_get_file(cx, err, s);
    free(path);
    return asg;
  } else {
    err->tag = OO_ERR_IMPORT;
    return NULL;
  }
}

void oo_context_free(OoContext *cx) {
  raxFree(cx->features);
  raxFree(cx->files);
  free(cx->mods_mod); // TODO this needs to be freed recursively
  free(cx->deps_mod); // TODO does this need recursive free as well?
}
