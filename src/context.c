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

bool is_type_binding(AsgBinding b) {
  return b.tag == BINDING_TYPE ||
  b.tag == BINDING_SUM_TYPE ||
  b.tag == BINDING_TYPE_VAR ||
  b.tag == BINDING_PRIMITIVE;
}

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
    case OO_ERR_BINDING_NOT_TYPE:
      printf("%s\n", "binding is not a type error");
      break;
    case OO_ERR_BINDING_NOT_EXP:
      printf("%s\n", "binding is not an expression error");
      break;
    case OO_ERR_DUP_ID_SCOPE:
      printf("%s\n", "duplicate id in scope error");
      break;
    case OO_ERR_BINDING_NOT_SUMMAND:
      printf("%s\n", "binding is not a summand error");
      break;
    case OO_ERR_NOT_CONST_EXP:
      printf("%s\n", "invalid top level val (not a constant expression) error");
      break;
    case OO_ERR_WRONG_NUMBER_OF_TYPE_ARGS:
      printf("%s\n", "wrong number of type args error");
      break;
    case OO_ERR_HIGHER_ORDER_TYPE_ARG:
      printf("%s\n", "higher order type arg error");
      break;
    case OO_ERR_NAMED_TYPE_APP_SID:
      printf("%s\n", "named type application sid error");
      break;
  }

  if (err->tag != OO_ERR_NONE && err->tag != OO_ERR_SYNTAX && err->tag != OO_ERR_FILE) {
    printf("In file %s\n", err->asg->path);
  }

  switch (err->tag) {
    case OO_ERR_NONE:
    case OO_ERR_FILE:
      break;
    case OO_ERR_SYNTAX:
      printf("In file %s\n", err->parser.path);
      switch (err->parser.tag) {
        case ERR_NONE:
          abort();
          break;
        case ERR_SID:
          printf("syntax error parsing a simple identifier\n");
          break;
        case ERR_ID:
          printf("syntax error parsing an identifier\n");
          break;
        case ERR_MACRO_INV:
          printf("syntax error parsing a macro invocation\n");
          break;
        case ERR_LITERAL:
          printf("syntax error parsing a literal\n");
          break;
        case ERR_SIZE_OF:
          printf("syntax error parsing a sizeof\n");
          break;
        case ERR_ALIGN_OF:
          printf("syntax error parsing an alignof\n");
          break;
        case ERR_REPEAT:
          printf("syntax error parsing a repeat\n");
          break;
        case ERR_BIN_OP:
          printf("syntax error parsing a binary operator\n");
          break;
        case ERR_ASSIGN_OP:
          printf("syntax error parsing an assingment operator\n");
          break;
        case ERR_TYPE:
          printf("syntax error parsing a type\n");
          break;
        case ERR_SUMMAND:
          printf("syntax error parsing a summand\n");
          break;
        case ERR_PATTERN:
          printf("syntax error parsing a pattern\n");
          break;
        case ERR_EXP:
          printf("syntax error parsing an expression\n");
          break;
        case ERR_BLOCK:
          printf("syntax error parsing a block\n");
          break;
        case ERR_ATTR:
          printf("syntax error parsing an attribute\n");
          break;
        case ERR_META:
          printf("syntax error parsing a meta item\n");
          break;
        case ERR_USE_TREE:
          printf("syntax error parsing a use tree\n");
          break;
        case ERR_ITEM:
          printf("syntax error parsing an item\n");
          break;
        case ERR_FILE:
          printf("syntax error parsing a file\n");
          break;
      }
      if (token_type_error(err->parser.tt) != NULL) {
        printf("%s\n", token_type_error(err->parser.tt));
      } else {
        printf("Unexpected token: %s\n", token_type_name(err->parser.tt));
      }
      print_location(str_new(err->parser.src, 0), str_new(err->parser.full_src, strlen(err->parser.full_src)));
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
    case OO_ERR_BINDING_NOT_TYPE:
      print_location(err->binding_not_type->str, err->asg->str);
      str_print(err->binding_not_type->str);
      break;
    case OO_ERR_BINDING_NOT_EXP:
      print_location(err->binding_not_exp->str, err->asg->str);
      str_print(err->binding_not_exp->str);
      break;
    case OO_ERR_DUP_ID_SCOPE:
      print_location(err->dup_id_scope, err->asg->str);
      str_print(err->dup_id_scope);
      break;
    case OO_ERR_BINDING_NOT_SUMMAND:
      print_location(err->binding_not_summand->str, err->asg->str);
      str_print(err->binding_not_summand->str);
      break;
    case OO_ERR_NOT_CONST_EXP:
      print_location(err->not_const_exp->str, err->asg->str);
      str_print(err->not_const_exp->str);
      break;
    case OO_ERR_WRONG_NUMBER_OF_TYPE_ARGS:
      print_location(err->wrong_number_of_type_args->str, err->asg->str);
      str_print(err->wrong_number_of_type_args->str);
      break;
    case OO_ERR_HIGHER_ORDER_TYPE_ARG:
      print_location(err->higher_order_type_arg->str, err->asg->str);
      str_print(err->higher_order_type_arg->str);
      break;
    case OO_ERR_NAMED_TYPE_APP_SID:
      print_location(err->named_type_app_sid->str, err->asg->str);
      str_print(err->named_type_app_sid->str);
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
  ns->bindings[0].file = NULL;
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
        inner_binding->file = NULL;
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
          err->parser.path = inner_path;
          goto done;
        }
        asg->path = inner_path;

        // oo_filter_cc(asg, features); FIXME filtering changes the addresses of asg nodes, breaking bindings, frees and everything... Solution: Add asg nodes that represent filtered nodes

        inner_binding->tag = BINDING_NS;
        inner_binding->private = false;
        inner_binding->file = asg;
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
        use->sid.binding.file = asg;

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
  sb_add(asg->ns.bindings, 18 + (int) count); // mod, dep, and the 14 primitive types
  asg->ns.bindings_by_sid = raxNew();
  asg->ns.pub_bindings_by_sid = raxNew();

  asg->ns.bindings[0].tag = BINDING_NS;
  asg->ns.bindings[0].private = true;
  asg->ns.bindings[0].file = NULL;
  asg->ns.bindings[0].ns = cx->dirs[0];
  raxInsert(asg->ns.bindings_by_sid, "mod", 3, &asg->ns.bindings[0], NULL);

  asg->ns.bindings[1].tag = BINDING_NS;
  asg->ns.bindings[1].private = true;
  asg->ns.bindings[1].file = NULL;
  asg->ns.bindings[1].ns = cx->dirs[1];
  raxInsert(asg->ns.bindings_by_sid, "dep", 3, &asg->ns.bindings[1], NULL);

  asg->ns.bindings[2].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[2].private = false;
  asg->ns.bindings[2].file = NULL;
  asg->ns.bindings[2].primitive = PRIM_U8;
  raxInsert(asg->ns.bindings_by_sid, "U8", 2, &asg->ns.bindings[2], NULL);

  asg->ns.bindings[3].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[3].private = false;
  asg->ns.bindings[3].file = NULL;
  asg->ns.bindings[3].primitive = PRIM_U16;
  raxInsert(asg->ns.bindings_by_sid, "U16", 3, &asg->ns.bindings[3], NULL);

  asg->ns.bindings[4].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[4].private = false;
  asg->ns.bindings[4].file = NULL;
  asg->ns.bindings[4].primitive = PRIM_U32;
  raxInsert(asg->ns.bindings_by_sid, "U32", 3, &asg->ns.bindings[4], NULL);

  asg->ns.bindings[5].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[5].private = false;
  asg->ns.bindings[5].file = NULL;
  asg->ns.bindings[5].primitive = PRIM_U64;
  raxInsert(asg->ns.bindings_by_sid, "U64", 3, &asg->ns.bindings[5], NULL);

  asg->ns.bindings[6].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[6].private = false;
  asg->ns.bindings[6].file = NULL;
  asg->ns.bindings[6].primitive = PRIM_USIZE;
  raxInsert(asg->ns.bindings_by_sid, "Usize", 5, &asg->ns.bindings[6], NULL);

  asg->ns.bindings[7].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[7].private = false;
  asg->ns.bindings[7].file = NULL;
  asg->ns.bindings[7].primitive = PRIM_I8;
  raxInsert(asg->ns.bindings_by_sid, "I8", 2, &asg->ns.bindings[7], NULL);

  asg->ns.bindings[8].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[8].private = false;
  asg->ns.bindings[8].file = NULL;
  asg->ns.bindings[8].primitive = PRIM_I16;
  raxInsert(asg->ns.bindings_by_sid, "I16", 3, &asg->ns.bindings[8], NULL);

  asg->ns.bindings[9].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[9].private = false;
  asg->ns.bindings[9].file = NULL;
  asg->ns.bindings[9].primitive = PRIM_I32;
  raxInsert(asg->ns.bindings_by_sid, "I32", 3, &asg->ns.bindings[9], NULL);

  asg->ns.bindings[10].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[10].private = false;
  asg->ns.bindings[10].file = NULL;
  asg->ns.bindings[10].primitive = PRIM_I64;
  raxInsert(asg->ns.bindings_by_sid, "I64", 3, &asg->ns.bindings[10], NULL);

  asg->ns.bindings[11].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[11].private = false;
  asg->ns.bindings[11].file = NULL;
  asg->ns.bindings[11].primitive = PRIM_ISIZE;
  raxInsert(asg->ns.bindings_by_sid, "Isize", 5, &asg->ns.bindings[11], NULL);

  asg->ns.bindings[12].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[12].private = false;
  asg->ns.bindings[12].file = NULL;
  asg->ns.bindings[12].primitive = PRIM_F32;
  raxInsert(asg->ns.bindings_by_sid, "F32", 3, &asg->ns.bindings[12], NULL);

  asg->ns.bindings[13].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[13].private = false;
  asg->ns.bindings[13].file = NULL;
  asg->ns.bindings[13].primitive = PRIM_F64;
  raxInsert(asg->ns.bindings_by_sid, "F64", 3, &asg->ns.bindings[13], NULL);

  asg->ns.bindings[14].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[14].private = false;
  asg->ns.bindings[14].file = NULL;
  asg->ns.bindings[14].primitive = PRIM_VOID;
  raxInsert(asg->ns.bindings_by_sid, "Void", 4, &asg->ns.bindings[14], NULL);

  asg->ns.bindings[15].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[15].private = false;
  asg->ns.bindings[15].file = NULL;
  asg->ns.bindings[15].primitive = PRIM_BOOL;
  raxInsert(asg->ns.bindings_by_sid, "Bool", 4, &asg->ns.bindings[15], NULL);

  asg->ns.bindings[16].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[16].private = false;
  asg->ns.bindings[16].file = NULL;
  asg->ns.bindings[16].primitive = PRIM_U128;
  raxInsert(asg->ns.bindings_by_sid, "U128", 4, &asg->ns.bindings[16], NULL);

  asg->ns.bindings[17].tag = BINDING_PRIMITIVE;
  asg->ns.bindings[17].private = false;
  asg->ns.bindings[17].file = NULL;
  asg->ns.bindings[17].primitive = PRIM_I128;
  raxInsert(asg->ns.bindings_by_sid, "I128", 4, &asg->ns.bindings[17], NULL);

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
            asg->ns.bindings[i + 18].tag = BINDING_TYPE; // later overwritten for sum types
            asg->ns.bindings[i + 18].private = true;
            asg->ns.bindings[i + 18].file = asg;
            asg->ns.bindings[i + 18].type = &asg->items[i].type; // later overwritten for sum types

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
                sum->ns.bindings[j].tag = BINDING_VAL;
                sum->ns.bindings[j].private = true;
                sum->ns.bindings[j].file = asg;
                sum->ns.bindings[j].val.mut = false;
                sum->ns.bindings[j].val.sid = &sum->summands[j - 1].sid;
                sum->ns.bindings[j].val.type = NULL;
                sum->ns.bindings[j].val.oo_type.tag = OO_TYPE_UNINITIALIZED;
                sum->ns.bindings[j].val.tag = VAL_SUMMAND;
                sum->ns.bindings[j].val.summand = &sum->summands[j - 1];

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

              asg->ns.bindings[i + 18].tag = BINDING_SUM_TYPE;
              asg->ns.bindings[i + 18].sum.type = &asg->items[i];
              asg->ns.bindings[i + 18].sum.ns = &sum->ns;
            }

            break;
          case ITEM_VAL:
            str = asg->items[i].val.sid.str;
            asg->ns.bindings[i + 18].tag = BINDING_VAL;
            asg->ns.bindings[i + 18].private = true;
            asg->ns.bindings[i + 18].file = asg;
            asg->ns.bindings[i + 18].val.mut = asg->items[i].val.mut;
            asg->ns.bindings[i + 18].val.sid = &asg->items[i].val.sid;
            asg->ns.bindings[i + 18].val.type = &asg->items[i].val.type;
            asg->ns.bindings[i + 18].val.oo_type.tag = OO_TYPE_UNINITIALIZED;
            asg->ns.bindings[i + 18].val.tag = VAL_VAL;
            asg->ns.bindings[i + 18].val.val = &asg->items[i].val;
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            asg->ns.bindings[i + 18].tag = BINDING_VAL;
            asg->ns.bindings[i + 18].private = true;
            asg->ns.bindings[i + 18].file = asg;
            asg->ns.bindings[i + 18].val.mut = false;
            asg->ns.bindings[i + 18].val.sid = &asg->items[i].fun.sid;
            asg->ns.bindings[i + 18].val.type = NULL;
            asg->ns.bindings[i + 18].val.oo_type.tag = OO_TYPE_UNINITIALIZED;
            asg->ns.bindings[i + 18].val.tag = VAL_FUN;
            asg->ns.bindings[i + 18].val.fun = &asg->items[i].fun;
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            asg->ns.bindings[i + 18].tag = BINDING_VAL;
            asg->ns.bindings[i + 18].private = true;
            asg->ns.bindings[i + 18].file = asg;
            asg->ns.bindings[i + 18].val.mut = asg->items[i].ffi_val.mut;
            asg->ns.bindings[i + 18].val.sid = &asg->items[i].ffi_val.sid;
            asg->ns.bindings[i + 18].val.type = &asg->items[i].ffi_val.type;
            asg->ns.bindings[i + 18].val.oo_type.tag = OO_TYPE_UNINITIALIZED;
            asg->ns.bindings[i + 18].val.tag = VAL_FFI;
            asg->ns.bindings[i + 18].val.ffi = &asg->items[i].ffi_val;
            break;
          default:
            abort(); // unreachable
        }

        if (raxInsert(asg->ns.bindings_by_sid, str.start, str.len, &asg->ns.bindings[i + 18], NULL)) {
          if (asg->items[i].pub) {
            raxInsert(asg->ns.pub_bindings_by_sid, str.start, str.len, &asg->ns.bindings[i + 18], NULL);
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

typedef struct ScopeStackFrame {
  rax *bindings; // AsgBindings by their sid
  AsgBinding **owned_bindings; // Stretchy buffer of owning ptrs to bindings
  bool owns_its_rax; // Whether freeing the rax in this frame is te ScopeStack's responsibility
} ScopeStackFrame;

// Nested scopes (mappings from sids to AsgBindings).
// Allows to manipulate the stack of scopes, and to look up bindings, by
// traversing the scopes from the innermost to the outermost one.
// Does not own any data.
typedef struct ScopeStack {
  ScopeStackFrame *frames; // stretchy buffer of stack frames.
  size_t len; // how many items are on the stack
} ScopeStack;

static void ss_init(ScopeStack *ss) {
  ss->frames = NULL;
  ss->len = 0;
}

static void ss_free(const ScopeStack *ss) {
  for (size_t i = 0; i < ss->len; i++) {
    for (int j = 0; j < sb_count(ss->frames[i].owned_bindings); j++) {
      free(ss->frames[i].owned_bindings[j]);
    }
    sb_free(ss->frames[i].owned_bindings);
    if (ss->frames[i].owns_its_rax) {
      raxFree(ss->frames[i].bindings);
    }
  }
  sb_free(ss->frames);
}

static void ss_push(ScopeStack *ss, rax *scope) {
  ScopeStackFrame f;
  f.bindings = scope;
  f.owned_bindings = NULL;
  f.owns_its_rax = false;

  if (sb_count(ss->frames) <= (int) ss->len) {
    sb_push(ss->frames, f);
  } else {
    ss->frames[ss->len] = f;
  }
  ss->len += 1;
}

static void ss_push_owning(ScopeStack *ss, rax *scope) {
  ScopeStackFrame f;
  f.bindings = scope;
  f.owned_bindings = NULL;
  f.owns_its_rax = true;

  if (sb_count(ss->frames) <= (int) ss->len) {
    sb_push(ss->frames, f);
  } else {
    ss->frames[ss->len] = f;
  }
  ss->len += 1;
}

static void ss_pop(ScopeStack *ss) {
  ss->len -= 1;

  for (int i = 0; i < sb_count(ss->frames[ss->len].owned_bindings); i++) {
    free(ss->frames[ss->len].owned_bindings[i]);
  }
  sb_free(ss->frames[ss->len].owned_bindings);
  if (ss->frames[ss->len].owns_its_rax) {
    raxFree(ss->frames[ss->len].bindings);
  }
}

// Add a binding to the current scope, free it when the scope gets popped.
static void ss_add(OoError *err, AsgFile *asg, ScopeStack *ss, Str sid, AsgBinding *binding /* owning */) {
  sb_push(ss->frames[ss->len - 1].owned_bindings, binding);
  if (raxInsert(ss->frames[ss->len - 1].bindings, sid.start, sid.len, binding, NULL) == 0) {
    err->tag = OO_ERR_DUP_ID_SCOPE;
    err->asg = asg;
    err->dup_id_scope = sid;
    return;
  }
}

// Resolve the sid to a binding, return NULL if none is found.
static AsgBinding *ss_get(const ScopeStack *ss, Str sid) {
  for (size_t i = ss->len; i > 0; i--) {
    AsgBinding *b = raxFind(ss->frames[i - 1].bindings, sid.start, sid.len);
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
static void summand_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgSummand *summand, AsgFile *asg);
static void exp_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgExp *exp, AsgFile *asg);
static void block_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgBlock *block, AsgFile *asg);

// bindings and sids are pointers to stretchy buffers, bindings is an sb of owning pointers
void add_pattern_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgPattern *p, AsgFile *asg) {
  size_t count;
  switch (p->tag) {
    case PATTERN_ID:
      if (p->id.type != NULL) {
        type_fine_bindings(cx, err, ss, p->id.type, asg);
      }

      AsgBinding *b = malloc(sizeof(AsgBinding));
      b->tag = BINDING_VAL;
      b->private = true;
      b->file = asg;
      b->val.mut = p->id.mut;
      b->val.sid = &p->id.sid;
      b->val.type = p->id.type;
      b->val.oo_type.tag = OO_TYPE_UNINITIALIZED;
      b->val.tag = VAL_PATTERN;
      b->val.pattern = &p->id;

      ss_add(err, asg, ss, p->id.sid.str, b);
      break;
    case PATTERN_BLANK:
    case PATTERN_LITERAL:
      // noop
      break;
    case PATTERN_PTR:
      add_pattern_bindings(cx, err, ss, p->ptr, asg);
      break;
    case PATTERN_PRODUCT_ANON:
      count = sb_count(p->product_anon);
      for (size_t i = 0; i < count; i++) {
        add_pattern_bindings(cx, err, ss, &p->product_anon[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_PRODUCT_NAMED:
      count = sb_count(p->product_named.inners);
      for (size_t i = 0; i < count; i++) {
        add_pattern_bindings(cx, err, ss, &p->product_named.inners[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_SUMMAND_ANON:
      id_fine_bindings(cx, err, ss, &p->summand_anon.id, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      } else {
        if (p->summand_anon.id.binding.tag != BINDING_VAL && p->summand_anon.id.binding.val.tag != VAL_SUMMAND) {
          err->tag = OO_ERR_BINDING_NOT_SUMMAND;
          err->binding_not_summand = &p->summand_anon.id;
          return;
        }
      }

      count = sb_count(p->summand_anon.fields);
      for (size_t i = 0; i < count; i++) {
        add_pattern_bindings(cx, err, ss, &p->summand_anon.fields[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_SUMMAND_NAMED:
      id_fine_bindings(cx, err, ss, &p->summand_named.id, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      } else {
        if (p->summand_anon.id.binding.tag != BINDING_VAL && p->summand_anon.id.binding.val.tag != VAL_SUMMAND) {
          err->tag = OO_ERR_BINDING_NOT_SUMMAND;
          err->binding_not_summand = &p->summand_named.id;
          return;
        }
      }

      count = sb_count(p->summand_named.fields);
      for (size_t i = 0; i < count; i++) {
        add_pattern_bindings(cx, err, ss, &p->summand_named.fields[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
}

void oo_cx_fine_bindings(OoContext *cx, OoError *err) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    file_fine_bindings(cx, err, cx->files[i]);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}

// Return whether the expression can be assigned to a top level val item.
static bool is_item_val(AsgExp *exp) {
  switch (exp->tag) {
    case EXP_ID:
    case EXP_LITERAL:
    case EXP_SIZE_OF:
    case EXP_ALIGN_OF:
      return true;
    case EXP_REF:
      return is_item_val(exp->ref);
    case EXP_REF_MUT:
      return is_item_val(exp->ref_mut);
    case EXP_DEREF:
      return is_item_val(exp->deref);
    case EXP_DEREF_MUT:
      return is_item_val(exp->deref_mut);
    case EXP_ARRAY:
      return is_item_val(exp->array);
    case EXP_ARRAY_INDEX:
      return is_item_val(exp->array_index.arr) && is_item_val(exp->array_index.index);
    case EXP_PRODUCT_REPEATED:
      return is_item_val(exp->product_repeated.inner);
    case EXP_PRODUCT_ANON:
      for (size_t i = 0; i < (size_t) sb_count(exp->product_anon); i++) {
        if (!is_item_val(&exp->product_anon[i])) {
          return false;
        }
      }
      return true;
    case EXP_PRODUCT_NAMED:
      for (size_t i = 0; i < (size_t) sb_count(exp->product_named.inners); i++) {
        if (!is_item_val(&exp->product_named.inners[i])) {
          return false;
        }
      }
      return true;
    case EXP_PRODUCT_ACCESS_ANON:
      return is_item_val(exp->product_access_anon.inner);
    case EXP_PRODUCT_ACCESS_NAMED:
      return is_item_val(exp->product_access_named.inner);
    case EXP_CAST:
      return is_item_val(exp->cast.inner);
    case EXP_NOT:
      return is_item_val(exp->exp_not);
    case EXP_NEGATE:
      return is_item_val(exp->exp_negate);
    case EXP_WRAPPING_NEGATE:
      return is_item_val(exp->exp_wrapping_negate);
    case EXP_BIN_OP:
      return is_item_val(exp->bin_op.lhs) && is_item_val(exp->bin_op.rhs);
    default:
      return false;
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
        if (is_item_val(&asg->items[i].val.exp)) {
          type_fine_bindings(cx, err, &ss, &asg->items[i].val.type, asg);
          if (err->tag != OO_ERR_NONE) {
            ss_free(&ss);
            return;
          }
          exp_fine_bindings(cx, err, &ss, &asg->items[i].val.exp, asg);
        } else {
          err->tag = OO_ERR_NOT_CONST_EXP;
          err->not_const_exp = &asg->items[i].val.exp;
          ss_free(&ss);
          return;
        }
        break;
      case ITEM_FUN:
        if (sb_count(asg->items[i].fun.type_args) > 0) {
          ss_push_owning(&ss, raxNew());

          for (size_t j = 0; j < (size_t) sb_count(asg->items[i].fun.type_args); j++) {
            AsgBinding *b = malloc(sizeof(AsgBinding));
            b->tag = BINDING_TYPE_VAR;
            b->private = true;
            b->file = asg;
            b->type_var = &asg->items[i].fun.type_args[j];
            ss_add(err, asg, &ss, asg->items[i].fun.type_args[j].str, b);
            if (err->tag != OO_ERR_NONE) {
              ss_free(&ss);
              return;
            }
          }
        }

        for (size_t j = 0; j < (size_t) sb_count(asg->items[i].fun.arg_types); j++) {
          type_fine_bindings(cx, err, &ss, &asg->items[i].fun.arg_types[j], asg);
          if (err->tag != OO_ERR_NONE) {
            ss_free(&ss);
            return;
          }
        }

        type_fine_bindings(cx, err, &ss, &asg->items[i].fun.ret, asg);
        if (err->tag != OO_ERR_NONE) {
          ss_free(&ss);
          return;
        }

        if (sb_count(asg->items[i].fun.arg_types) > 0) {
          ss_push_owning(&ss, raxNew());

          for (size_t j = 0; j < (size_t) sb_count(asg->items[i].fun.arg_types); j++) {
            AsgBinding *b = malloc(sizeof(AsgBinding));
            b->tag = BINDING_VAL;
            b->private = true;
            b->file = asg;
            b->val.mut = asg->items[i].fun.arg_muts[j];
            b->val.sid = &asg->items[i].fun.arg_sids[j];
            b->val.type = &asg->items[i].fun.arg_types[j];
            b->val.oo_type.tag = OO_TYPE_UNINITIALIZED;
            b->val.tag = VAL_ARG;
            b->val.arg = &asg->items[i].fun.arg_sids[j];

            ss_add(err, asg, &ss, asg->items[i].fun.arg_sids[j].str, b);
            if (err->tag != OO_ERR_NONE) {
              ss_free(&ss);
              return;
            }
          }
        }

        block_fine_bindings(cx, err, &ss, &asg->items[i].fun.body, asg);

        if (sb_count(asg->items[i].fun.arg_types) > 0) {
          ss_pop(&ss);
        }

        if (sb_count(asg->items[i].fun.type_args) > 0) {
          ss_pop(&ss);
        }
        break;
      case ITEM_FFI_VAL:
        type_fine_bindings(cx, err, &ss, &asg->items[i].ffi_val.type, asg);
        break;
      case ITEM_USE:
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }

    if (err->tag != OO_ERR_NONE) {
      ss_free(&ss);
      return;
    }
  }

  ss_free(&ss);
}

static void type_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgType *type, AsgFile *asg) {
  size_t count;
  switch (type->tag) {
    case TYPE_ID:
      id_fine_bindings(cx, err, ss, &type->id, asg);
      if (err->tag == OO_ERR_NONE && !is_type_binding(type->id.binding)) {
        err->tag = OO_ERR_BINDING_NOT_TYPE;
        err->binding_not_type = &type->id;
        return;
      }
      break;
    case TYPE_MACRO:
      abort(); // macros get are evaluated before binding resolution
      break;
    case TYPE_PTR:
      type_fine_bindings(cx, err, ss, type->ptr, asg);
      break;
    case TYPE_PTR_MUT:
      type_fine_bindings(cx, err, ss, type->ptr_mut, asg);
      break;
    case TYPE_ARRAY:
      type_fine_bindings(cx, err, ss, type->array, asg);
      break;
    case TYPE_PRODUCT_REPEATED:
      type_fine_bindings(cx, err, ss, type->product_repeated.inner, asg);
      break;
    case TYPE_PRODUCT_ANON:
      count = sb_count(type->product_anon);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->product_anon[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_PRODUCT_NAMED:
      count = sb_count(type->product_named.types);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->product_named.types[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_FUN_ANON:
      count = sb_count(type->fun_anon.args);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->fun_anon.args[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      type_fine_bindings(cx, err, ss, type->fun_anon.ret, asg);
      break;
    case TYPE_FUN_NAMED:
      count = sb_count(type->fun_named.arg_types);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->fun_named.arg_types[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      type_fine_bindings(cx, err, ss, type->fun_named.ret, asg);
      break;
    case TYPE_APP_ANON:
      id_fine_bindings(cx, err, ss, &type->app_anon.tlf, asg);
      if (err->tag == OO_ERR_NONE && !is_type_binding(type->app_anon.tlf.binding)) {
        err->tag = OO_ERR_BINDING_NOT_TYPE;
        err->binding_not_type = &type->app_anon.tlf;
        return;
      }

      count = sb_count(type->app_anon.args);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->app_anon.args[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_APP_NAMED:
      id_fine_bindings(cx, err, ss, &type->app_named.tlf, asg);
      if (err->tag == OO_ERR_NONE && !is_type_binding(type->app_named.tlf.binding)) {
        err->tag = OO_ERR_BINDING_NOT_TYPE;
        err->binding_not_type = &type->app_named.tlf;
        return;
      }

      count = sb_count(type->app_named.types);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &type->app_named.types[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_GENERIC:
      ss_push_owning(ss, raxNew());

      for (size_t i = 0; i < (size_t) sb_count(type->generic.args); i++) {
        AsgBinding *b = malloc(sizeof(AsgBinding));
        b->tag = BINDING_TYPE_VAR;
        b->private = true;
        b->file = asg;
        b->type_var = &type->generic.args[i];
        ss_add(err, asg, ss, type->generic.args[i].str, b);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }

      type_fine_bindings(cx, err, ss, type->generic.inner, asg);
      ss_pop(ss);
      break;
    case TYPE_SUM:
      count = sb_count(type->sum.summands);
      for (size_t i = 0; i < count; i++) {
        summand_fine_bindings(cx, err, ss, &type->sum.summands[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
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

  id->binding = id->sids[count - 1].binding;
}

static void summand_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgSummand *summand, AsgFile *asg) {
  size_t count;
  switch (summand->tag) {
    case SUMMAND_ANON:
      count = sb_count(summand->anon);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &summand->anon[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case SUMMAND_NAMED:
      count = sb_count(summand->named.inners);
      for (size_t i = 0; i < count; i++) {
        type_fine_bindings(cx, err, ss, &summand->named.inners[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
}

static void exp_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgExp *exp, AsgFile *asg) {
  size_t count;
  switch (exp->tag) {
    case EXP_ID:
      id_fine_bindings(cx, err, ss, &exp->id, asg);
      if (err->tag == OO_ERR_NONE && exp->id.binding.tag != BINDING_VAL) {
        err->tag = OO_ERR_BINDING_NOT_EXP;
        err->binding_not_exp = &exp->id;
        return;
      }
      break;
    case EXP_MACRO:
      abort(); // macros are evaluated before binding resolution
      break;
    case EXP_LITERAL:
      // noop
      break;
    case EXP_REF:
      exp_fine_bindings(cx, err, ss, exp->ref, asg);
      break;
    case EXP_REF_MUT:
      exp_fine_bindings(cx, err, ss, exp->ref_mut, asg);
      break;
    case EXP_DEREF:
      exp_fine_bindings(cx, err, ss, exp->deref, asg);
      break;
    case EXP_DEREF_MUT:
      exp_fine_bindings(cx, err, ss, exp->deref_mut, asg);
      break;
    case EXP_ARRAY:
      exp_fine_bindings(cx, err, ss, exp->array, asg);
      break;
    case EXP_ARRAY_INDEX:
      exp_fine_bindings(cx, err, ss, exp->array_index.arr, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
      exp_fine_bindings(cx, err, ss, exp->array_index.index, asg);
      break;
    case EXP_PRODUCT_REPEATED:
      exp_fine_bindings(cx, err, ss, exp->product_repeated.inner, asg);
      break;
    case EXP_PRODUCT_ANON:
      count = sb_count(exp->product_anon);
      for (size_t i = 0; i < count; i++) {
        exp_fine_bindings(cx, err, ss, &exp->product_anon[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_PRODUCT_NAMED:
      count = sb_count(exp->product_named.inners);
      for (size_t i = 0; i < count; i++) {
        exp_fine_bindings(cx, err, ss, &exp->product_named.inners[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_PRODUCT_ACCESS_ANON:
      exp_fine_bindings(cx, err, ss, exp->product_access_anon.inner, asg);
      break;
    case EXP_PRODUCT_ACCESS_NAMED:
      exp_fine_bindings(cx, err, ss, exp->product_access_named.inner, asg);
      break;
    case EXP_FUN_APP_ANON:
      exp_fine_bindings(cx, err, ss, exp->fun_app_anon.fun, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->fun_app_anon.args);
      for (size_t i = 0; i < count; i++) {
        exp_fine_bindings(cx, err, ss, &exp->fun_app_anon.args[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_FUN_APP_NAMED:
      exp_fine_bindings(cx, err, ss, exp->fun_app_named.fun, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->fun_app_named.args);
      for (size_t i = 0; i < count; i++) {
        exp_fine_bindings(cx, err, ss, &exp->fun_app_named.args[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_CAST:
      exp_fine_bindings(cx, err, ss, exp->cast.inner, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      type_fine_bindings(cx, err, ss, exp->cast.type, asg);
      break;
    case EXP_SIZE_OF:
      type_fine_bindings(cx, err, ss, exp->size_of, asg);
      break;
    case EXP_ALIGN_OF:
      type_fine_bindings(cx, err, ss, exp->align_of, asg);
      break;
    case EXP_NOT:
      exp_fine_bindings(cx, err, ss, exp->exp_not, asg);
      break;
    case EXP_NEGATE:
      exp_fine_bindings(cx, err, ss, exp->exp_negate, asg);
      break;
    case EXP_WRAPPING_NEGATE:
      exp_fine_bindings(cx, err, ss, exp->exp_wrapping_negate, asg);
      break;
    case EXP_BIN_OP:
      exp_fine_bindings(cx, err, ss, exp->bin_op.lhs, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      exp_fine_bindings(cx, err, ss, exp->bin_op.rhs, asg);
      break;
    case EXP_ASSIGN:
      exp_fine_bindings(cx, err, ss, exp->assign.lhs, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      exp_fine_bindings(cx, err, ss, exp->assign.rhs, asg);
      break;
    case EXP_VAL:
      add_pattern_bindings(cx, err, ss, &exp->val, asg);
      break;
    case EXP_VAL_ASSIGN:
      exp_fine_bindings(cx, err, ss, exp->val_assign.rhs, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      add_pattern_bindings(cx, err, ss, &exp->val_assign.lhs, asg);
      break;
    case EXP_BLOCK:
      block_fine_bindings(cx, err, ss, &exp->block, asg);
      break;
    case EXP_IF:
      exp_fine_bindings(cx, err, ss, exp->exp_if.cond, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      block_fine_bindings(cx, err, ss, &exp->exp_if.if_block, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      block_fine_bindings(cx, err, ss, &exp->exp_if.else_block, asg);
      break;
    case EXP_CASE:
      exp_fine_bindings(cx, err, ss, exp->exp_case.matcher, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->exp_case.patterns);
      for (size_t i = 0; i < count; i++) {
        ss_push_owning(ss, raxNew());
        add_pattern_bindings(cx, err, ss, &exp->exp_case.patterns[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        block_fine_bindings(cx, err, ss, &exp->exp_case.blocks[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        ss_pop(ss);
      }
      break;
    case EXP_WHILE:
      exp_fine_bindings(cx, err, ss, exp->exp_while.cond, asg);
      block_fine_bindings(cx, err, ss, &exp->exp_while.block, asg);
      break;
    case EXP_LOOP:
      exp_fine_bindings(cx, err, ss, exp->exp_loop.matcher, asg);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->exp_loop.patterns);
      for (size_t i = 0; i < count; i++) {
        ss_push_owning(ss, raxNew());
        add_pattern_bindings(cx, err, ss, &exp->exp_loop.patterns[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        block_fine_bindings(cx, err, ss, &exp->exp_loop.blocks[i], asg);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        ss_pop(ss);
      }
      break;
    case EXP_RETURN:
      if (exp->exp_return != NULL) {
        exp_fine_bindings(cx, err, ss, exp->exp_return, asg);
      }
      break;
    case EXP_BREAK:
      if (exp->exp_break != NULL) {
        exp_fine_bindings(cx, err, ss, exp->exp_break, asg);
      }
      break;
    case EXP_GOTO:
    case EXP_LABEL:
      // we can compile to C without properly resolving these... It's ugly, but
      // it works for the temporary compiler.
      // TODO don't be lazy, implement proper bindings for labels and goto...
      break;
  }
}

static void block_fine_bindings(OoContext *cx, OoError *err, ScopeStack *ss, AsgBlock *block, AsgFile *asg) {
  for (size_t i = 0; i < (size_t) sb_count(block->exps); i++) {
    exp_fine_bindings(cx, err, ss, &block->exps[i], asg);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}
