#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE true
#endif

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "context.h"
#include "parser.h"
#include "rax.h"
#include "cc.h"
#include "stretchy_buffer.h"
#include "util.h"

void err_print(OoError *err) {
  switch (err->tag) {
    case OO_ERR_NONE:
      printf("%s\n", "no error");
      break;
    case OO_ERR_SYNTAX:
      printf("%s\n", "syntax error");
      break;
    case OO_ERR_FILE:
      printf("%s\n", "file error");
      break;
    case OO_ERR_CYCLIC_IMPORTS:
      printf("%s\n", "cyclic imports error");
      break;
    case OO_ERR_DUP_ID_ITEM:
      printf("%s\n", "duplicate item id error");
      break;
    case OO_ERR_DUP_ID_ITEM_USE:
      printf("%s\n", "duplicate item id due to use error");
      break;
    case OO_ERR_INVALID_BRANCH:
      printf("%s\n", "invalid branch error");
      break;
    case OO_ERR_NONEXISTING_SID_USE:
      printf("%s\n", "nonexisting sid use error");
      break;
  }
}

static bool is_ns_uninitialized(AsgNS *ns) {
  return ns->bindings == NULL;
}

static bool is_ns_fully_initialized(AsgNS *ns) {
  return (uint) sb_count(ns->bindings) == raxSize(ns->bindings_by_sid);
}

static bool is_ns_initializing(AsgNS *ns) {
  return !is_ns_uninitialized(ns) && !is_ns_fully_initialized(ns);
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
  size_t path_len = strlen(path);
  char *inner_path;

  // printf("handle dir: %s\n", path);

  dp = opendir(path);
  if (dp == NULL) {
    err->tag = OO_ERR_FILE;
    err->file = path;
    return;
  }

  size_t dir_len = 0;
  while ((ep = readdir(dp))) {
    dir_len += 1;
  }
  rewinddir(dp);
  dir_len -= 2; // . and ..
  sb_add(ns->bindings, (int) dir_len + 1); // ns->bindings[0] is pointer to self

  ns->bindings[0].tag = BINDING_NS;
  ns->bindings[0].ns = ns;
  raxInsert(ns->bindings_by_sid, "mod", 3, (void *) &ns->bindings[0], NULL);

  size_t i = 1;
  while ((ep = readdir(dp))) {
    if (
      (strlen(ep->d_name) == 1 && strcmp(ep->d_name, ".") == 0) ||
      (strlen(ep->d_name) == 2 && strcmp(ep->d_name, "..") == 0)
    ) {
      continue;
    }
    AsgBinding *inner_binding = &ns->bindings[i];
    i += 1;
    inner_path = malloc(path_len + 1 + strlen(ep->d_name) + 1);
    strcpy(inner_path, path);
    inner_path[path_len] = '/';
    strcpy(inner_path + (path_len + 1), ep->d_name);

    // printf("  ep->d_name: %s\n", ep->d_name);

    AsgNS *dir_ns;
    FILE *f;
    char **src;
    AsgFile *asg;
    switch (ep->d_type) {
      case DT_DIR:
        sb_push(cx->dirs, malloc(sizeof(AsgNS)));
        dir_ns = cx->dirs[sb_count(cx->dirs) - 1];
        dir_ns->bindings = NULL;
        dir_ns->bindings_by_sid = raxNew();
        dir_ns->pub_bindings_by_sid = NULL;
        dir_ns->tag = NS_DIR;

        inner_binding->tag = BINDING_NS;
        inner_binding->ns = dir_ns;
        // printf("    %s\n", inner_path);
        // printf("    %s\n", ep->d_name);
        raxInsert(ns->bindings_by_sid, ep->d_name, strlen(ep->d_name), (void *) inner_binding, NULL);
        // raxShow(ns->bindings_by_sid);
        // printf("\n");

        parse_handle_dir(inner_path, dir_ns, cx, err, features);
        free(inner_path);
        if (err->tag != OO_ERR_NONE) {
          goto done;
        }
        break;
      case DT_REG:
        // printf("  %s\n", "regular file");
        // printf("  inner_path: %s\n", inner_path);
        f = fopen(inner_path, "r");
        if (f == NULL) {
          err->tag = OO_ERR_FILE;
          err->file = inner_path;
          goto done;
        }
        if (fseek(f, 0, SEEK_END)) {
          err->tag = OO_ERR_FILE;
          err->file = inner_path;
          fclose(f);
          goto done;
        }
        long fsize = ftell(f);
        if (fsize < 0) {
          err->tag = OO_ERR_FILE;
          err->file = inner_path;
          fclose(f);
          goto done;
        }
        rewind(f);

        src = sb_add(cx->sources, 1);
        *src = malloc(fsize + 1);
        fread(*src, fsize, 1, f);
        if (ferror(f)) {
          err->tag = OO_ERR_FILE;
          err->file = inner_path;
          fclose(f);
          goto done;
        }
        (*src)[fsize] = 0;

        if (fclose(f)) {
          err->tag = OO_ERR_FILE;
          err->file = inner_path;
          goto done;
        }
        // printf("  %s\n", "file ops done");
        // end file handling

        sb_push(cx->files, malloc(sizeof(AsgFile)));
        asg = cx->files[sb_count(cx->files) - 1];

        parse_file(*src, &err->parser, asg);
        if (err->parser.tag != ERR_NONE) {
          err->tag = OO_ERR_SYNTAX;
          goto done;
        }
        asg->path = inner_path;

        oo_filter_cc(asg, features);

        inner_binding->tag = BINDING_NS;
        inner_binding->ns = &asg->ns;
        raxInsert(ns->bindings_by_sid, ep->d_name, strlen(ep->d_name) - 3 /* removes the .oo extension*/, (void *) inner_binding, NULL);
        break;
      default:
        err->tag = OO_ERR_FILE;
        err->file = inner_path;
        goto done;
    }
  }

  assert(is_ns_fully_initialized(ns));

  done:
    closedir(dp);
}

void oo_cx_parse(OoContext *cx, OoError *err, rax *features) {
  sb_add(cx->dirs, 2); // mod and dep dirs

  cx->dirs[0] = malloc(sizeof(AsgNS));
  cx->dirs[0]->bindings = NULL;
  cx->dirs[0]->bindings_by_sid = raxNew();
  cx->dirs[0]->pub_bindings_by_sid = NULL;
  cx->dirs[0]->tag = NS_MODS;

  cx->dirs[1] = malloc(sizeof(AsgNS));
  cx->dirs[1]->bindings = NULL;
  cx->dirs[1]->bindings_by_sid = raxNew();
  cx->dirs[1]->pub_bindings_by_sid = NULL;
  cx->dirs[1]->tag = NS_DEPS;

  parse_handle_dir(cx->mods, cx->dirs[0], cx, err, features);
  if (err->tag != OO_ERR_NONE) {
    return;
  }

  parse_handle_dir(cx->deps, cx->dirs[1], cx, err, features);
}

void oo_cx_free(OoContext *cx) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    free_inner_file(*(cx->files[i]));
    free(cx->files[i]);
  }
  sb_free(cx->files);

  count = sb_count(cx->dirs);
  for (int i = 0; i < count; i++) {
    free_ns(*(cx->dirs[i]));
    free(cx->dirs[i]);
  }
  sb_free(cx->dirs);

  count = sb_count(cx->sources);
  for (int i = 0; i < count; i++) {
    free((void *) cx->sources[i]);
  }
  sb_free(cx->sources);
}

static void file_coarse_bindings(OoContext *cx, OoError *err, AsgFile *asg);
static void dir_coarse_bindings(OoContext *cx, OoError *err, AsgNS *dir);
static void resolve_use(AsgUseTree *use, bool pub, AsgNS *ns, AsgNS *parent, Str parent_name, OoContext *cx, OoError *err);

void oo_cx_coarse_bindings(OoContext *cx, OoError *err) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    if (is_ns_uninitialized(&cx->files[i]->ns)) {
      file_coarse_bindings(cx, err, cx->files[i]);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
    }
  }
}

static void prepare_file(OoContext *cx, OoError *err, AsgFile *file) {
  if (is_ns_initializing(&file->ns)) {
    err->tag = OO_ERR_CYCLIC_IMPORTS;
    err->cyclic_imports = file;
    return;
  } else if (is_ns_uninitialized(&file->ns)) {
    file_coarse_bindings(cx, err, file);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}

// (Recursively) add all bindings from the given UseTree to the mod.
static void resolve_use(
  AsgUseTree *use,
  bool pub,
  AsgNS *ns, /* where to put the resolved bindings */
  AsgNS *parent, /* where to look for the sid */
  Str parent_name,
  OoContext *cx,
  OoError *err) {
    AsgBinding *b = raxFind(parent->bindings_by_sid, use->sid.str.start, use->sid.str.len);

    // printf("resolve use for ");
    // str_print(use->str);
    // printf(" in %s\n", use->asg->path);

    if (b == raxNotFound) {
      err->tag = OO_ERR_NONEXISTING_SID_USE;
      err->nonexisting_sid_use = use;
      // printf("unfound str: ");
      // str_print(use->sid.str);
      // raxShow(parent->bindings_by_sid);
      // printf("actual addr: %p\n", (void *) ns);
      // printf("actual addr of asg: %p\n", (void *) use->asg);
      return;
    }

    if (b->tag == BINDING_NS && b->ns->tag == NS_FILE) {
      prepare_file(cx, err, b->ns->file);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
    }

    Str str = use->sid.str;
    int count;
    switch (use->tag) {
      case USE_TREE_RENAME:
        // printf("rename: ");
        // str_print(use->rename.str);
        str = use->rename.str;
        __attribute__((fallthrough));
      case USE_TREE_LEAF:
        // printf("inserting ");
        // str_print(str);

        if (str_eq_parts(str, "mod", 3) && parent_name.len > 0) {
          str = parent_name;
          b = &parent->bindings[0];
        }

        if (raxInsert(ns->bindings_by_sid, str.start, str.len, b, NULL)) {
          if (pub && ns->tag == NS_FILE) {
            raxInsert(ns->pub_bindings_by_sid, str.start, str.len, b, NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID_ITEM_USE;
          err->dup_item_use = use;
          return;
        }
        break;
      case USE_TREE_BRANCH:
        if (b->tag != BINDING_NS && b->tag != BINDING_SUM_TYPE) {
          err->tag = OO_ERR_INVALID_BRANCH;
          err->invalid_branch = use;
          return;
        }

        count = sb_count(use->branch);
        for (int i = 0; i < count; i++) {
          if (b->tag == BINDING_NS) {
            resolve_use(&use->branch[i], pub, ns, b->ns, str, cx, err);
          } else {
            // BINDING_SUM_TYPE
            resolve_use(&use->branch[i], pub, ns, b->sum.ns, str, cx, err);
          }
          if (err->tag != OO_ERR_NONE) {
            return;
          }
        }
        break;
    }
}

static void file_coarse_bindings(OoContext *cx, OoError *err, AsgFile *asg) {
  // printf("file_coarse_bindings for %s\n", asg->path);

  size_t count = sb_count(asg->items);
  sb_add(asg->ns.bindings, 2 + (int) count);
  asg->ns.bindings_by_sid = raxNew();
  asg->ns.pub_bindings_by_sid = raxNew();

  asg->ns.bindings[0].tag = BINDING_NS;
  asg->ns.bindings[0].ns = cx->dirs[0];
  raxInsert(asg->ns.bindings_by_sid, "mod", 3, &asg->ns.bindings[0], NULL);

  asg->ns.bindings[1].tag = BINDING_NS;
  asg->ns.bindings[1].ns = cx->dirs[1];
  raxInsert(asg->ns.bindings_by_sid, "dep", 3, &asg->ns.bindings[1], NULL);

  for (size_t i = 0; i < count; i++) {
    Str str;
    switch (asg->items[i].tag) {
      case ITEM_TYPE:
      case ITEM_VAL:
      case ITEM_FUN:
      case ITEM_FFI_VAL:
        switch (asg->items[i].tag) {
          case ITEM_TYPE:
            str = asg->items[i].type.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_TYPE; // later overwritten for sum types
            asg->ns.bindings[i + 2].type = &asg->items[i]; // later overwritten for sum types

            // handle sum type namespaces
            bool is_sum = false;
            AsgTypeSum *sum;

            if (asg->items[i].type.type.tag == TYPE_SUM) {
              is_sum = true;
              sum = &asg->items[i].type.type.sum;
            } else if (asg->items[i].type.type.tag == TYPE_GENERIC && asg->items[i].type.type.generic.inner->tag == TYPE_SUM) {
              is_sum = true;
              sum = &asg->items[i].type.type.generic.inner->sum;
            }

            if (is_sum) {
              sum->ns.tag = NS_SUM;
              sum->ns.sum = sum;

              sum->ns.bindings_by_sid = raxNew();
              sum->ns.pub_bindings_by_sid = raxNew();
              sum->ns.bindings = NULL;
              int count = sb_count(sum->summands);
              sb_add(sum->ns.bindings, count);

              for (int j = 0; j < count; j++) {
                raxInsert(
                  sum->ns.bindings_by_sid,
                  sum->summands[j].sid.str.start,
                  sum->summands[j].sid.str.len,
                  &sum->ns.bindings[j],
                  NULL
                );

                if (sum->pub) {
                  raxInsert(
                    sum->ns.pub_bindings_by_sid,
                    sum->summands[j].sid.str.start,
                    sum->summands[j].sid.str.len,
                    &sum->ns.bindings[j],
                    NULL
                  );
                }
              }

              asg->ns.bindings[i + 2].tag = BINDING_SUM_TYPE;
              asg->ns.bindings[i + 2].sum.type = &asg->items[i];
              asg->ns.bindings[i + 2].sum.ns = &sum->ns;
            }

            break;
          case ITEM_VAL:
            str = asg->items[i].val.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_VAL;
            asg->ns.bindings[i + 2].val = &asg->items[i];
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_FUN;
            asg->ns.bindings[i + 2].fun = &asg->items[i];
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_FFI_VAL;
            asg->ns.bindings[i + 2].ffi_val = &asg->items[i];
            break;
          default:
            abort(); // unreachable
        }

        if (raxInsert(asg->ns.bindings_by_sid, str.start, str.len, &asg->ns.bindings[i + 2], NULL)) {
          if (asg->items[i].pub) {
            raxInsert(asg->ns.pub_bindings_by_sid, str.start, str.len, &asg->ns.bindings[i + 2], NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID_ITEM;
          err->dup_item = &asg->items[i];
          return;
        }

        break;
      case ITEM_USE:
        // printf("hi: %s\n", asg->path);
        // str_print(asg->items[i].use.str);
        // printf("%zu\n", i);
        resolve_use(&asg->items[i].use, asg->items[i].pub, &asg->ns, &asg->ns, str_new(NULL, 0), cx, err);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        break;
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }
  }
}
