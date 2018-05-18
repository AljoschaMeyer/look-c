// Conditional compilation: This provides the implementation of oo_filter_cc.
#include <stdbool.h>
#include <string.h>

#include "asg.h"
#include "parser.h"
#include "rax.h"
#include "stretchy_buffer.h"

static bool should_stay(AsgMeta *attrs, rax *features) {
  int i;
  int count = sb_count(attrs);
  for (i = 0; i < count; i++) {
    if (attrs[i].tag == META_UNARY && attrs[i].name_len == 2 && strncmp(attrs[i].name, "cc", 2) == 0) {
      if (attrs[i].unary.tag == LITERAL_STRING &&
        raxFind(features, (unsigned char*) attrs[i].unary.src + 1, attrs[i].unary.len - 2) == raxNotFound) {
        return false;
      }
    }
  }

  return true;
}

static void filter_block(AsgBlock *block, rax *features) {
  AsgMeta **new_attrs = NULL;
  AsgExp *new_exps = NULL;

  int i;
  size_t copied = 0;
  int count = sb_count(block->exps);
  for (i = 0; i < count; i++) {
    if (should_stay(block->attrs[i], features)) {
      sb_add(new_attrs, 1);
      new_attrs[copied] = block->attrs[i];
      sb_add(new_exps, 1);
      new_exps[copied] = block->exps[i];

      switch (new_exps[copied].tag) {
        case EXP_IF:
          filter_block(&new_exps[copied].exp_if.if_block, features);
          filter_block(&new_exps[copied].exp_if.else_block, features);
          break;
        case EXP_WHILE:
          filter_block(&new_exps[copied].exp_while.block, features);
          break;
        case EXP_CASE:
          ;
          int count_case = sb_count(new_exps[copied].exp_case.blocks);
          for (int j_case = 0; j_case < count_case; j_case++) {
            filter_block(&new_exps[copied].exp_case.blocks[j_case], features);
          }
          break;
        case EXP_LOOP:
          ;
          int count_loop = sb_count(new_exps[copied].exp_loop.blocks);
          for (int j_loop = 0; j_loop < count_loop; j_loop++) {
            filter_block(&new_exps[copied].exp_loop.blocks[j_loop], features);
          }
          break;
        case EXP_BLOCK:
          filter_block(&new_exps[copied].block, features);
          break;
        default:
          break;
      }

      copied += 1;
    } else {
      free_sb_meta(block->attrs[i]);
      free_inner_exp(block->exps[i]);
    }
  }

  sb_free(block->attrs);
  block->attrs = new_attrs;
  sb_free(block->exps);
  block->exps = new_exps;
}

void oo_filter_cc(AsgFile *asg, rax *features) {
  AsgMeta **new_attrs = NULL;
  AsgItem *new_items = NULL;

  int i;
  size_t copied = 0;
  int count = sb_count(asg->items);
  for (i = 0; i < count; i++) {
    if (should_stay(asg->attrs[i], features)) {
      sb_add(new_attrs, 1);
      new_attrs[copied] = asg->attrs[i];
      sb_add(new_items, 1);
      new_items[copied] = asg->items[i];

      if (new_items[copied].tag == ITEM_FUN) {
        filter_block(&new_items[copied].fun.body, features);
      }
      copied += 1;
    } else {
      free_sb_meta(asg->attrs[i]);
      free_inner_item(asg->items[i]);
    }
  }

  sb_free(asg->attrs);
  asg->attrs = new_attrs;
  sb_free(asg->items);
  asg->items = new_items;
}
