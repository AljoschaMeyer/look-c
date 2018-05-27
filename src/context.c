#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE true
#endif

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include "context.h"
#include "parser.h"
#include "rax.h"
#include "cc.h"
#include "stretchy_buffer.h"

void free_ns(AsgNS ns) {
  if (ns.bindings_by_sid != NULL) {
    raxFree(ns.bindings_by_sid);
  }

  if (ns.pub_bindings_by_sid != NULL) {
    raxFree(ns.pub_bindings_by_sid);
  }

  sb_free(ns.bindings);
}

void oo_cx_init(OoContext *cx, const char *mods, const char *deps) {
  cx->mods = mods;
  cx->deps = deps;
  cx->files = NULL;
  cx->dirs = NULL;
  cx->sources = NULL;
}

static void parse_handle_dir(const char *path, AsgNS *ns, OoContext *cx, OoError *err, rax *features) {
  DIR *dp;
  struct dirent *ep;

  dp = opendir(path);
  if (dp == NULL) {
    err->tag = OO_ERR_FILE;
    err->file = path;
    return;
  }

  while ((ep = readdir(dp))) {
    const char *inner_name = basename(ep->d_name);
    AsgBinding *inner_binding = sb_add(ns->bindings, 1);

    AsgNS *dir_ns;
    FILE *f;
    char **src;
    AsgFile *asg;
    switch (ep->d_type) {
      case DT_DIR:
        dir_ns = sb_add(cx->dirs, 1);
        dir_ns->bindings = NULL;
        dir_ns->bindings_by_sid = raxNew();
        dir_ns->pub_bindings_by_sid = NULL;

        inner_binding->tag = BINDING_NS;
        inner_binding->ns = dir_ns;
        raxInsert(ns->bindings_by_sid, inner_name, strlen(inner_name), (void *) inner_binding, NULL);

        parse_handle_dir(ep->d_name, dir_ns, cx, err, features);
        if (err->tag != OO_ERR_NONE) {
          goto done;
        }
        break;
      case DT_REG:
        f = fopen(ep->d_name, "r");
        if (f == NULL) {
          err->tag = OO_ERR_FILE;
          err->file = ep->d_name;
          goto done;
        }
        if (fseek(f, 0, SEEK_END)) {
          err->tag = OO_ERR_FILE;
          err->file = ep->d_name;
          fclose(f);
          goto done;
        }
        long fsize = ftell(f);
        if (fsize < 0) {
          err->tag = OO_ERR_FILE;
          err->file = ep->d_name;
          fclose(f);
          goto done;
        }
        rewind(f);

        src = sb_add(cx->sources, 1);
        fread(*src, fsize, 1, f);
        if (ferror(f)) {
          err->tag = OO_ERR_FILE;
          err->file = ep->d_name;
          fclose(f);
          goto done;
        }
        *src[fsize] = 0;

        if (fclose(f)) {
          err->tag = OO_ERR_FILE;
          err->file = ep->d_name;
          goto done;
        }
        // end file handling

        asg = sb_add(cx->files, 1);
        parse_file(*src, &err->parser, asg);
        if (err->parser.tag != ERR_NONE) {
          err->tag = OO_ERR_SYNTAX;
          goto done;
        }

        oo_filter_cc(asg, features);

        inner_binding->tag = BINDING_NS;
        inner_binding->ns = &asg->ns;
        raxInsert(ns->bindings_by_sid, inner_name, strlen(inner_name) - 3 /* removes the .oo extension*/, (void *) inner_binding, NULL);
        break;
      default:
        err->tag = OO_ERR_FILE;
        err->file = ep->d_name;
        goto done;
    }
  }

  done:
    closedir(dp);
}

void oo_cx_parse(OoContext *cx, OoError *err, rax *features) {
  sb_add(cx->dirs, 2); // mod and dep dirs

  parse_handle_dir(cx->mods, &cx->dirs[0], cx, err, features);
  if (err->tag != OO_ERR_NONE) {
    return;
  }

  parse_handle_dir(cx->deps, &cx->dirs[1], cx, err, features);
}

void oo_cx_free(OoContext *cx) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    free_inner_file(cx->files[i]);
  }
  sb_free(cx->files);

  count = sb_count(cx->dirs);
  for (int i = 0; i < count; i++) {
    free_ns(cx->dirs[i]);
  }
  sb_free(cx->dirs);

  count = sb_count(cx->sources);
  for (int i = 0; i < count; i++) {
    free((void *) cx->sources[i]);
  }
  sb_free(cx->sources);
}

// AsgFile *oo_get_file(OoContext *cx, OoError *err, Str path) {
//   char *src = NULL;
//
//   void *f = raxFind(cx->files, path.start, path.len);
//   if (f == raxNotFound) {
//     FILE *f = fopen(path.start, "r");
//     if (f == NULL) {
//       goto file_err;
//     }
//     if (fseek(f, 0, SEEK_END)) {
//       goto file_err;
//     }
//     long fsize = ftell(f);
//     if (fsize < 0) {
//       goto file_err;
//     }
//     rewind(f);
//
//     src = malloc(fsize + 1);
//     fread(src, fsize, 1, f);
//     if (ferror(f)) {
//       goto file_err;
//     }
//     src[fsize] = 0;
//
//     if (fclose(f)) {
//       goto file_err;
//     }
//     // end file handling
//
//     AsgFile *asg = malloc(sizeof(AsgFile));
//     parse_file(src, &err->parser, asg);
//     if (err->parser.tag != ERR_NONE) {
//       free(src);
//       free(asg);
//       err->tag = OO_ERR_SYNTAX;
//       return NULL;
//     }
//
//     oo_filter_cc(asg, cx->features);
//
//     raxInsert(cx->files, path.start, path.len, asg, NULL);
//
//     err->tag = OO_ERR_NONE;
//     return asg;
//   } else {
//     err->tag = OO_ERR_NONE;
//     return (AsgFile *) f;
//   }
//
//   file_err:
//   free(src);
//   err->tag = OO_ERR_FILE;
//   return NULL;
// }
//
// AsgFile *oo_get_file_ids(OoContext *cx, OoError *err, Str *ids /* stretchy buffer */) {
//   if (sb_count(ids) < 2) {
//     err->tag = OO_ERR_IMPORT;
//     return NULL;
//   }
//   if (str_eq_parts(ids[0], "mod", 3)) {
//     size_t mods_len = strlen(cx->mods);
//     Str s;
//     s.len = mods_len;
//     size_t count = (size_t) sb_count(ids);
//     for (size_t i = 1; i < count; i++) {
//       s.len += 1; // "/"
//       s.len += ids[i].len;
//     }
//     s.len += 4; // ".oo" NULL
//
//     char *path = malloc(s.len);
//     strcpy(path, cx->mods);
//     size_t offset = mods_len;
//     for (size_t i = 1; i < count; i++) {
//       path[offset] = '/';
//       offset += 1;
//       strncpy(path + offset, ids[i].start, ids[i].len);
//       offset += ids[i].len;
//     }
//     memcpy(path + offset, ".oo", 4);
//     s.start = path;
//
//     AsgFile *asg = oo_get_file(cx, err, s);
//     free(path);
//     return asg;
//   } else if (str_eq_parts(ids[0], "dep", 3)) {
//     size_t deps_len = strlen(cx->deps);
//     Str s;
//     s.len = deps_len + 1 + ids[1].len + 8 /* "/lib.oo" NULL */;
//
//     char *path = malloc(s.len);
//     strcpy(path, cx->deps);
//     path[deps_len] = '/';
//     strncpy(path + deps_len + 1, ids[1].start, ids[1].len);
//     strncpy(path + deps_len + 1 + ids[1].len, "/lib.oo", 8);
//     s.start = path;
//
//     AsgFile *asg = oo_get_file(cx, err, s);
//     free(path);
//     return asg;
//   } else {
//     err->tag = OO_ERR_IMPORT;
//     return NULL;
//   }
// }
//
// void oo_context_free(OoContext *cx) {
//   raxFree(cx->features);
//   raxFree(cx->files);
//   free(cx->mods_mod); // TODO this needs to be freed recursively
//   free(cx->deps_mod); // TODO does this need recursive free as well?
// }
