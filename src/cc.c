// Conditional compilation: This provides the implementation of oo_filter_cc.
#include <stdbool.h>
#include <string.h>

#include "cc.h"
#include "parser.h"
#include "stretchy_buffer.h"

static bool should_stay(AsgMeta *attrs, rax *features) {
  int i;
  int count = sb_count(attrs);
  for (i = 0; i < count; i++) {
    if (attrs[i].tag == META_UNARY && str_eq_parts(attrs[i].name, "cc", 2)) {
      if (attrs[i].unary.tag == LITERAL_STRING &&
        raxFind(features, attrs[i].unary.str.start + 1, attrs[i].unary.str.len - 2) == raxNotFound) {
        return false;
      }
    }
  }

  return true;
}

static void filter_exp_sb(AsgExp *exps, rax *features);

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
      copied += 1;
    } else {
      free_sb_meta(block->attrs[i]);
      free_inner_exp(block->exps[i]);
    }

    filter_exp_sb(block->exps, features);
  }

  sb_free(block->attrs);
  block->attrs = new_attrs;
  sb_free(block->exps);
  block->exps = new_exps;
}

static void filter_block_sb(AsgBlock *blocks, rax *features) {
  int count = sb_count(blocks);
  for (int i = 0; i < count; i++) {
    filter_block(&blocks[i], features);
  }
}

static void filter_exp(AsgExp *exp, rax *features);

static void filter_exp_sb(AsgExp *exps, rax *features) {
  int count = sb_count(exps);
  for (int i = 0; i < count; i++) {
    filter_exp(&exps[i], features);
  }
}

static void filter_exp(AsgExp *exp, rax *features) {
  switch (exp->tag) {
    case EXP_REF:
      filter_exp(exp->ref, features);
      break;
    case EXP_REF_MUT:
      filter_exp(exp->ref_mut, features);
      break;
    case EXP_DEREF:
      filter_exp(exp->deref, features);
      break;
    case EXP_DEREF_MUT:
      filter_exp(exp->deref_mut, features);
      break;
    case EXP_ARRAY:
      filter_exp(exp->array, features);
      break;
    case EXP_ARRAY_INDEX:
      filter_exp(exp->array_index.arr, features);
      filter_exp(exp->array_index.index, features);
      break;
    case EXP_PRODUCT_REPEATED:
      filter_exp(exp->product_repeated.inner, features);
      break;
    case EXP_PRODUCT_ANON:
      filter_exp_sb(exp->product_anon, features);
      break;
    case EXP_PRODUCT_NAMED:
      filter_exp_sb(exp->product_named.inners, features);
      break;
    case EXP_PRODUCT_ACCESS_ANON:
      filter_exp(exp->product_access_anon.inner, features);
      break;
    case EXP_PRODUCT_ACCESS_NAMED:
      filter_exp(exp->product_access_named.inner, features);
      break;
    case EXP_FUN_APP_ANON:
      filter_exp(exp->fun_app_anon.fun, features);
      filter_exp_sb(exp->fun_app_anon.args, features);
      break;
    case EXP_FUN_APP_NAMED:
      filter_exp(exp->fun_app_named.fun, features);
      filter_exp_sb(exp->fun_app_named.args, features);
      break;
    case EXP_CAST:
      filter_exp(exp->cast.inner, features);
      break;
    case EXP_NOT:
      filter_exp(exp->exp_not, features);
      break;
    case EXP_NEGATE:
      filter_exp(exp->exp_negate, features);
      break;
    case EXP_BIN_OP:
      filter_exp(exp->bin_op.lhs, features);
      filter_exp(exp->bin_op.rhs, features);
      break;
    case EXP_ASSIGN:
      filter_exp(exp->assign.lhs, features);
      filter_exp(exp->assign.rhs, features);
      break;
    case EXP_VAL_ASSIGN:
      filter_exp(exp->assign.rhs, features);
      break;
    case EXP_BLOCK:
      filter_block(&exp->block, features);
      break;
    case EXP_IF:
      filter_exp(exp->exp_if.cond, features);
      filter_block(&exp->exp_if.if_block, features);
      filter_block(&exp->exp_if.else_block, features);
      break;
    case EXP_WHILE:
      filter_exp(exp->exp_while.cond, features);
      filter_block(&exp->exp_while.block, features);
      break;
    case EXP_CASE:
      filter_exp(exp->exp_case.matcher, features);
      filter_block_sb(exp->exp_case.blocks, features);
      break;
    case EXP_LOOP:
      filter_exp(exp->exp_loop.matcher, features);
      filter_block_sb(exp->exp_loop.blocks, features);
      break;
    case EXP_RETURN:
      if (exp->exp_return != NULL) {
        filter_exp(exp->exp_return, features);
      }
      break;
    case EXP_BREAK:
      if (exp->exp_break != NULL) {
        filter_exp(exp->exp_break, features);
      }
      break;
    default:
      break;
  }
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
