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

static void print_location(Str loc, Str file) {
  size_t line = 0;
  size_t col = 0;

  for (size_t i = 0; file.start + i != loc.start; i++) {
    if (i > file.len) {
      abort();
    }

    if (file.start[i] == '\n') {
      line += 1;
      col = 0;
    } else {
      col += 1;
    }
  }

  printf("line %zu, col %zu\n", line, col);
}

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
    case OO_ERR_NONEXISTING_SID:
      printf("%s\n", "nonexisting sid error");
      break;
    case OO_ERR_ID_NOT_A_NS:
      printf("%s\n", "id is not a namespace error");
      break;
    case OO_ERR_ID_NOT_IN_NS:
      printf("%s\n", "id is not in namespace error");
      break;
  }

  if (err->tag != OO_ERR_NONE && err->tag != OO_ERR_SYNTAX && err->tag != OO_ERR_FILE) {
    printf("In file %s\n", err->asg->path);
  }

  switch (err->tag) {
    case OO_ERR_NONE:
    case OO_ERR_SYNTAX:
    case OO_ERR_FILE:
      break;
    case OO_ERR_CYCLIC_IMPORTS:
      print_location(err->cyclic_import->str, err->asg->str);
      str_print(err->cyclic_import->str);
      break;
    case OO_ERR_DUP_ID_ITEM:
      print_location(err->dup_item->str, err->asg->str);
      str_print(err->dup_item->str);
      break;
    case OO_ERR_DUP_ID_ITEM_USE:
      print_location(err->dup_item_use->str, err->asg->str);
      str_print(err->dup_item_use->str);
      break;
    case OO_ERR_INVALID_BRANCH:
      print_location(err->invalid_branch->str, err->asg->str);
      str_print(err->invalid_branch->str);
      break;
    case OO_ERR_NONEXISTING_SID_USE:
      print_location(err->nonexisting_sid_use->str, err->asg->str);
      str_print(err->nonexisting_sid_use->str);
      break;
    case OO_ERR_NONEXISTING_SID:
      print_location(err->nonexisting_sid->str, err->asg->str);
      str_print(err->nonexisting_sid->str);
      break;
    case OO_ERR_ID_NOT_A_NS:
      print_location(err->id_not_a_ns->str, err->asg->str);
      str_print(err->id_not_a_ns->str);
      break;
    case OO_ERR_ID_NOT_IN_NS:
      print_location(err->id_not_in_ns->str, err->asg->str);
      str_print(err->id_not_in_ns->str);
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
  ns->bindings[0].private = true;
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
        inner_binding->private = true;
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
        inner_binding->private = false;
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
static void resolve_use(AsgUseTree *use, bool pub, AsgNS *ns, AsgNS *parent, Str parent_name, OoContext *cx, OoError *err, AsgFile *asg);

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

static void prepare_file(OoContext *cx, OoError *err, AsgFile *file, AsgSid *sid) {
  if (is_ns_initializing(&file->ns)) {
    err->tag = OO_ERR_CYCLIC_IMPORTS;
    err->cyclic_import = sid;
    err->asg = file;
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
  OoError *err,
  AsgFile *asg) {
    AsgBinding *b = raxFind(parent->bindings_by_sid, use->sid.str.start, use->sid.str.len);

    // printf("resolve use for ");
    // str_print(use->str);
    // printf(" in %s\n", use->asg->path);

    if (b == raxNotFound) {
      err->tag = OO_ERR_NONEXISTING_SID_USE;
      err->asg = asg;
      err->nonexisting_sid_use = use;
      // printf("unfound str: ");
      // str_print(use->sid.str);
      // raxShow(parent->bindings_by_sid);
      // printf("actual addr: %p\n", (void *) ns);
      // printf("actual addr of asg: %p\n", (void *) use->asg);
      return;
    }

    if (b->tag == BINDING_NS && b->ns->tag == NS_FILE) {
      prepare_file(cx, err, b->ns->file, &use->sid);
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

        memcpy(&use->sid.binding, b, sizeof(AsgBinding));
        use->sid.binding.private = false;

        if (raxInsert(ns->bindings_by_sid, str.start, str.len, &use->sid.binding, NULL)) {
          if (pub && ns->tag != NS_DIR) {
            raxInsert(ns->pub_bindings_by_sid, str.start, str.len, &use->sid.binding, NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID_ITEM_USE;
          err->asg = asg;
          err->dup_item_use = use;
          return;
        }
        break;
      case USE_TREE_BRANCH:
        if (b->tag != BINDING_NS && b->tag != BINDING_SUM_TYPE) {
          err->tag = OO_ERR_INVALID_BRANCH;
          err->asg = asg;
          err->invalid_branch = use;
          return;
        }

        count = sb_count(use->branch);
        for (int i = 0; i < count; i++) {
          if (b->tag == BINDING_NS) {
            resolve_use(&use->branch[i], pub, ns, b->ns, str, cx, err, asg);
          } else {
            assert(b->tag == BINDING_SUM_TYPE);
            resolve_use(&use->branch[i], pub, ns, b->sum.ns, str, cx, err, asg);
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
  asg->ns.bindings[0].private = true;
  asg->ns.bindings[0].ns = cx->dirs[0];
  raxInsert(asg->ns.bindings_by_sid, "mod", 3, &asg->ns.bindings[0], NULL);

  asg->ns.bindings[1].tag = BINDING_NS;
  asg->ns.bindings[1].private = true;
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
            asg->ns.bindings[i + 2].private = true;
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
              int count = sb_count(sum->summands) + 1;
              sb_add(sum->ns.bindings, count);

              sum->ns.bindings[0].tag = BINDING_SUM_TYPE;
              sum->ns.bindings[0].sum.type = &asg->items[i];
              sum->ns.bindings[0].sum.ns = &sum->ns;
              raxInsert(sum->ns.bindings_by_sid, "mod", 3, (void *) &sum->ns.bindings[0], NULL);
              raxInsert(sum->ns.pub_bindings_by_sid, "mod", 3, (void *) &sum->ns.bindings[0], NULL);

              for (int j = 1; j < count; j++) {
                sum->ns.bindings[j].tag = BINDING_SUMMAND;
                sum->ns.bindings[j].private = true;
                sum->ns.bindings[j].summand = &sum->summands[j - 1];

                raxInsert(
                  sum->ns.bindings_by_sid,
                  sum->summands[j - 1].sid.str.start,
                  sum->summands[j - 1].sid.str.len,
                  &sum->ns.bindings[j],
                  NULL
                );

                if (sum->pub) {
                  raxInsert(
                    sum->ns.pub_bindings_by_sid,
                    sum->summands[j - 1].sid.str.start,
                    sum->summands[j - 1].sid.str.len,
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
            asg->ns.bindings[i + 2].private = true;
            asg->ns.bindings[i + 2].val = &asg->items[i];
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_FUN;
            asg->ns.bindings[i + 2].private = true;
            asg->ns.bindings[i + 2].fun = &asg->items[i];
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            asg->ns.bindings[i + 2].tag = BINDING_FFI_VAL;
            asg->ns.bindings[i + 2].private = true;
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
          err->asg = asg;
          err->dup_item = &asg->items[i];
          return;
        }

        break;
      case ITEM_USE:
        // printf("hi: %s\n", asg->path);
        // str_print(asg->items[i].use.str);
        // printf("%zu\n", i);
        resolve_use(&asg->items[i].use, asg->items[i].pub, &asg->ns, &asg->ns, str_new(NULL, 0), cx, err, asg);
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

// Nested scopes (mappings from sids to AsgBindings).
// Allows to manipulate the stack of scopes, and to look up bindings, by
// traversing the scopes from the innermost to the outermost one.
// Does not own any data.
typedef struct ScopeStack {
  rax **scopes; // stretchy buffer of raxes of AsgBindings
  size_t len; // how many items are on the stack
} ScopeStack;

static void ss_init(ScopeStack *ss) {
  ss->scopes = NULL;
  ss->len = 0;
}

static void ss_free(const ScopeStack *ss) {
  sb_free(ss->scopes);
}

static void ss_push(ScopeStack *ss, rax *scope) {
  if (sb_count(ss->scopes) <= (int) ss->len) {
    sb_push(ss->scopes, scope);
  } else {
    ss->scopes[ss->len] = scope;
  }
  ss->len += 1;
}

static void ss_pop(ScopeStack *ss) {
  ss->len -= 1;
}

// Resolve the sid to a binding, return NULL if none is found.
static AsgBinding *ss_get(const ScopeStack *ss, Str sid) {
  for (size_t i = ss->len; i > 0; i--) {
    AsgBinding *b = raxFind(ss->scopes[i - 1], sid.start, sid.len);
    if (b != raxNotFound) {
      return b;
    }
  }
  return NULL;
}

// Returns a pointer to the ns of the binding, or NULL if the binding is not one
// to a namespace (i.e. neither BINDING_NS nor BINDING_SUM_TYPE).
static AsgNS *binding_get_ns(AsgBinding b) {
  switch (b.tag) {
    case BINDING_NS:
      return b.ns;
    case BINDING_SUM_TYPE:
      return b.sum.ns;
    default:
      return NULL;
  }
}

static void file_fine_bindings(OoContext *cx, OoError *err, AsgFile *asg);
static void type_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgType *type, AsgFile *asg);
static void id_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgId *id, AsgFile *asg);

void oo_cx_fine_bindings(OoContext *cx, OoError *err) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    file_fine_bindings(cx, err, cx->files[i]);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}

static void file_fine_bindings(OoContext *cx, OoError *err, AsgFile *asg) {
  ScopeStack ss;
  ss_init(&ss);
  ss_push(&ss, asg->ns.bindings_by_sid);

  size_t count = sb_count(asg->items);
  for (size_t i = 0; i < count; i++) {
    switch (asg->items[i].tag) {
      case ITEM_TYPE:
        type_fine_bindings(cx, err, &ss, &asg->items[i].type.type, asg);
        break;
      case ITEM_VAL:
      case ITEM_FUN:
      case ITEM_FFI_VAL:
        // TODO
        break;
      case ITEM_USE:
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }

    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }

  ss_free(&ss);
}

static void type_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgType *type, AsgFile *asg) {
  switch (type->tag) {
    case TYPE_ID:
      id_fine_bindings(cx, err, ss, &type->id, asg);
      break;
      // TODO resolve id, prepare file if needed
    default:
      // TODO handle everything explicitly
      break;
  }
  // TODO check errors after recursive calls
}

static void id_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgId *id, AsgFile *asg) {
  AsgBinding *base = ss_get(ss, id->sids[0].str);
  if (base == NULL) {
    err->tag = OO_ERR_NONEXISTING_SID;
    err->asg = asg;
    err->nonexisting_sid = &id->sids[0];
    return;
  }

  if (base->tag == BINDING_NS && base->ns->tag == NS_FILE) {
    prepare_file(cx, err, base->ns->file, &id->sids[0]);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }

  id->sids[0].binding = *base;

  size_t count = sb_count(id->sids);
  for (size_t i = 1; i < count; i++) {
    AsgNS *ns = binding_get_ns(id->sids[i - 1].binding);
    if (ns == NULL) {
      err->tag = OO_ERR_ID_NOT_A_NS;
      err->asg = asg;
      err->id_not_a_ns = &id->sids[i - 1];
      return;
    }

    rax *scope = ns->pub_bindings_by_sid;
    if (id->sids[i - 1].binding.private) {
      scope = ns->bindings_by_sid;
    }

    AsgBinding *b = raxFind(scope, id->sids[i].str.start, id->sids[i].str.len);
    if (b == raxNotFound) {
      err->tag = OO_ERR_ID_NOT_IN_NS;
      err->asg = asg;
      err->id_not_in_ns = &id->sids[i];
      return;
    }

    id->sids[i].binding = *b;
  }
}
