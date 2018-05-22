#include <stdio.h> // TODO remove

#include "asg.h"
#include "context.h"
#include "rax.h"
#include "stretchy_buffer.h"
#include "util.h"

void oo_init_item_maps(AsgFile *asg, OoContext *cx, OoError *err) {
  asg->did_init_items_by_sid = true;
  asg->items_by_sid = raxNew();
  asg->pub_items_by_sid = raxNew();

  size_t count = sb_count(asg->items);
  for (size_t i = 0; i < count; i++) {
    switch (asg->items[i].tag) {
      case ITEM_TYPE:
      case ITEM_VAL:
      case ITEM_FUN:
      case ITEM_FFI_VAL:
        ;
        Str str;
        switch (asg->items[i].tag) {
          case ITEM_TYPE:
            str = asg->items[i].type.sid.str;
            break;
          case ITEM_VAL:
            str = asg->items[i].val.sid.str;
            break;
          case ITEM_FUN:
            str = asg->items[i].fun.sid.str;
            break;
          case ITEM_FFI_VAL:
            str = asg->items[i].ffi_val.sid.str;
            break;
          default:
            abort();
        }

        if (raxInsert(asg->items_by_sid, str.start, str.len, &asg->items[i], NULL)) {
          if (asg->items[i].pub) {
            raxInsert(asg->pub_items_by_sid, str.start, str.len, &asg->items[i], NULL);
          }
        } else {
          err->tag = OO_ERR_DUP_ID;
          err->dup = str;
          return;
        }

        break;
      case ITEM_USE:
        printf("%s\n", cx->mods);
        // TODO impl
        break;
      case ITEM_FFI_INCLUDE:
        // noop
        break;
    }
  }


}
