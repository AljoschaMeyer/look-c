#include "asg.h"
#include "context.h"
#include "rax.h"
#include "stretchy_buffer.h"
#include "util.h"

static void init_handle_use_tree(AsgUseTree *use, OoContext *cx, OoError *err);

void oo_init_item_maps(AsgFile *asg, OoContext *cx, OoError *err) {
  asg->did_init_bindings_by_sid = true;
  asg->bindings_by_sid = raxNew();
  asg->pub_bindings_by_sid = raxNew();
  asg->bindings = NULL;
  size_t count = sb_count(asg->items);
  sb_add(asg->bindings, (int) count);

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
            asg->bindings[i].tag = BINDING_TYPE;
            asg->bindings[i].type = &asg->items[i];
            break;
          case ITEM_VAL:
            str = asg->items[i].val.sid.str;
            asg->bindings[i].tag = BINDING_VAL;
            asg->bindings[i].val = &asg->items[i];
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            asg->bindings[i].tag = BINDING_FUN;
            asg->bindings[i].fun = &asg->items[i];
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            asg->bindings[i].tag = BINDING_FFI_VAL;
            asg->bindings[i].ffi_val = &asg->items[i];
            break;
          default:
            abort();
        }

        if (raxInsert(asg->bindings_by_sid, str.start, str.len, &asg->bindings[i], NULL)) {
          if (asg->items[i].pub) {
            raxInsert(asg->pub_bindings_by_sid, str.start, str.len, &asg->bindings[i], NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID;
          err->dup = str;
          return;
        }

        break;
      case ITEM_USE:
        init_handle_use_tree(asg->items[i].use, NULL, cx, err);
        break;
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }
  }
}

static void init_handle_use_tree(AsgUseTree *use, Str *path, OoContext *cx, OoError *err) {
  // TODO
}
