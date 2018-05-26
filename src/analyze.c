#include "asg.h"
#include "context.h"
#include "rax.h"
#include "stretchy_buffer.h"
#include "util.h"

static void resolve_use(AsgUseTree *use, bool pub, AsgMod *mod, AsgMod *parent_mod, OoContext *cx, OoError *err);

void oo_init_mods(AsgFile *asg, OoContext *cx, OoError *err) {
  asg->did_init_mod = true;
  asg->mod.bindings_by_sid = raxNew();
  asg->mod.pub_bindings_by_sid = raxNew();
  asg->mod.bindings = NULL;
  size_t count = sb_count(asg->items);
  sb_add(asg->mod.bindings, (int) count);

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
            asg->mod.bindings[i].tag = BINDING_TYPE;
            asg->mod.bindings[i].type = &asg->items[i];
            break;
          case ITEM_VAL:
            str = asg->items[i].val.sid.str;
            asg->mod.bindings[i].tag = BINDING_VAL;
            asg->mod.bindings[i].val = &asg->items[i];
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            asg->mod.bindings[i].tag = BINDING_FUN;
            asg->mod.bindings[i].fun = &asg->items[i];
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            asg->mod.bindings[i].tag = BINDING_FFI_VAL;
            asg->mod.bindings[i].ffi_val = &asg->items[i];
            break;
          default:
            abort();
        }

        if (raxInsert(asg->mod.bindings_by_sid, str.start, str.len, &asg->mod.bindings[i], NULL)) {
          if (asg->items[i].pub) {
            raxInsert(asg->mod.pub_bindings_by_sid, str.start, str.len, &asg->mod.bindings[i], NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID;
          err->dup = str;
          return;
        }

        break;
      case ITEM_USE:
        ;
        AsgMod *parent_mod;
        if (str_eq_parts(asg->items[i].use.sid.str, "dep", 3)) {
          if (asg->items[i].use.tag != USE_TREE_BRANCH) {
            err->tag = OO_ERR_UNEXPECTED_DEP;
            return;
          }

          if (cx->deps_mod == NULL) {
            abort();
            // TODO init deps_mod
          }
          parent_mod = cx->deps_mod;
        } else if (str_eq_parts(asg->items[i].use.sid.str, "mod", 3)) {
          if (asg->items[i].use.tag != USE_TREE_BRANCH) {
            err->tag = OO_ERR_UNEXPECTED_MOD;
            return;
          }

          if (cx->mods_mod == NULL) {
            abort();
            // TODO init mods_mod
          }
          parent_mod = cx->mods_mod;
        } else {
          parent_mod = &asg->mod;
        }
        resolve_use(&asg->items[i].use, asg->items[i].pub, &asg->mod, parent_mod, cx, err);
        break;
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }
  }
}

// (Recursively) add all bindings from the given UseTree to the mod.
static void resolve_use(AsgUseTree *use, bool pub, AsgMod *mod, AsgMod *parent_mod, OoContext *cx, OoError *err) {
  AsgBinding *b;

  if (use->tag != USE_TREE_BRANCH && str_eq_parts(use->sid.str, "mod", 3)) {
    b = malloc(sizeof(AsgBinding));
    b->tag = BINDING_MOD; // XXX what about sum types?
    b->mod = parent_mod;
  } else {
    b = raxFind(parent_mod->bindings_by_sid, use->sid.str.start, use->sid.str.len);
  }

  if (b == raxNotFound) {
    err->tag = OO_ERR_NONEXISTING_SID;
    err->nonexisting_sid = use->sid.str;
    return;
  }

  Str str = use->sid.str;
  int count;
  switch (use->tag) {
    case USE_TREE_RENAME:
      str = use->rename.str;
      __attribute__((fallthrough));
    case USE_TREE_LEAF:
      if (raxInsert(mod->bindings_by_sid, str.start, str.len, b, NULL)) {
        if (pub) {
          raxInsert(mod->pub_bindings_by_sid, str.start, str.len, b, NULL);
        }
      } else {
        err->tag = OO_ERR_DUP_ID;
        err->dup = use->sid.str;
        return;
      }
      break;
    case USE_TREE_BRANCH:
      if (b->tag != BINDING_MOD) { // XXX what about sum types?
        err->tag = OO_ERR_INVALID_BRANCH;
        err->invalid_branch = use->sid.str;
        return;
      }

      count = sb_count(use->branch);
      for (int i = 0; i < count; i++) {
        resolve_use(&use->branch[i], pub, mod, b->mod, cx, err);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }

  if (use->tag != USE_TREE_BRANCH && str_eq_parts(use->sid.str, "mod", 3)) {
    free(b);
  }
}
