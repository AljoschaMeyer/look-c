#include <assert.h>

#include "asg.h"
#include "stretchy_buffer.h"
#include "context.h"

// The kind of a type is the number of type argument it takes. Since only kind 0
// types can be passed as type arguments, this single number is sufficient.
static uint32_t kind_arity(AsgType *type) {
  if (type->tag == TYPE_GENERIC) {
    return (size_t) sb_count(type->generic.args);
  } else {
    return 0;
  }
}

// Return the arity of the type behind this binding. Error if not a valid binding.
static uint32_t binding_kind_arity(OoError *err, AsgBinding b) {
  switch (b.tag) {
    case BINDING_TYPE:
      return kind_arity(&b.type->type);
    case BINDING_SUM_TYPE:
      return kind_arity(&b.sum.type->type.type);
    case BINDING_TYPE_VAR:
      return binding_kind_arity(err, b.type_var->binding);
    case BINDING_PRIMITIVE:
      return 0;
    default:
      err->tag = OO_ERR_BINDING_NOT_TYPE;
      return 42;
  }
}

// Return the arity of the type behind this binding. Error if not a valid (type) binding.
static uint32_t id_kind_arity(OoError *err, AsgId *id) {
  size_t ar = binding_kind_arity(err, id->binding);
  if (err->tag != OO_ERR_NONE) {
    err->binding_not_type = id;
  }
  return ar;
}

static void file_kind_checking(OoContext *cx, OoError *err, AsgFile *asg);
static void type_kind_checking(OoContext *cx, OoError *err, AsgType *type);
static void summand_kind_checking(OoContext *cx, OoError *err, AsgSummand *summand);
static void exp_kind_checking(OoContext *cx, OoError *err, AsgExp *exp);
static void pattern_kind_checking(OoContext *cx, OoError *err, AsgPattern *p);
static void block_kind_checking(OoContext *cx, OoError *err, AsgBlock *block);

static void file_kind_checking(OoContext *cx, OoError *err, AsgFile *asg) {
  err->asg = asg;
  size_t count = sb_count(asg->items);
  for (size_t i = 0; i < count; i++) {
    switch (asg->items[i].tag) {
      case ITEM_TYPE:
        type_kind_checking(cx, err, &asg->items[i].type.type);
        break;
      case ITEM_VAL:
        type_kind_checking(cx, err, &asg->items[i].val.type);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        exp_kind_checking(cx, err, &asg->items[i].val.exp);
        break;
      case ITEM_FUN:
        for (size_t j = 0; j < (size_t) sb_count(asg->items[i].fun.arg_types); j++) {
          type_kind_checking(cx, err, &asg->items[i].fun.arg_types[j]);
          if (err->tag != OO_ERR_NONE) {
            return;
          }
        }

        type_kind_checking(cx, err, &asg->items[i].fun.ret);
        if (err->tag != OO_ERR_NONE) {
          return;
        }

        block_kind_checking(cx, err, &asg->items[i].fun.body);
        break;
      case ITEM_FFI_VAL:
        type_kind_checking(cx, err, &asg->items[i].ffi_val.type);
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
}

static void type_kind_checking(OoContext *cx, OoError *err, AsgType *type) {
  size_t count;
  size_t tlf_kind;
  switch (type->tag) {
    case TYPE_ID:
    case TYPE_MACRO:
      // noop
      break;
    case TYPE_PTR:
      type_kind_checking(cx, err, type->ptr);
      break;
    case TYPE_PTR_MUT:
      type_kind_checking(cx, err, type->ptr_mut);
      break;
    case TYPE_ARRAY:
      type_kind_checking(cx, err, type->array);
      break;
    case TYPE_PRODUCT_REPEATED:
      type_kind_checking(cx, err, type->product_repeated.inner);
      break;
    case TYPE_PRODUCT_ANON:
      count = sb_count(type->product_anon);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &type->product_anon[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_PRODUCT_NAMED:
      count = sb_count(type->product_named.types);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &type->product_named.types[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case TYPE_FUN_ANON:
      count = sb_count(type->fun_anon.args);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &type->fun_anon.args[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      type_kind_checking(cx, err, type->fun_anon.ret);
      break;
    case TYPE_FUN_NAMED:
      count = sb_count(type->fun_named.arg_types);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &type->fun_named.arg_types[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      type_kind_checking(cx, err, type->fun_named.ret);
      break;
    case TYPE_APP_ANON:
      tlf_kind = id_kind_arity(err, &type->app_anon.tlf);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
      if (tlf_kind != (size_t) sb_count(type->app_anon.args)) {
        err->tag = OO_ERR_WRONG_NUMBER_OF_TYPE_ARGS;
        err->wrong_number_of_type_args = type;
        return;
      }

      count = sb_count(type->app_anon.args);
      for (size_t i = 0; i < count; i++) {
        if (kind_arity(&type->app_anon.args[i]) != 0) {
          err->tag = OO_ERR_HIGHER_ORDER_TYPE_ARG;
          err->higher_order_type_arg = &type->app_anon.args[i];
          return;
        }
      }
      break;
    case TYPE_APP_NAMED:
      tlf_kind = id_kind_arity(err, &type->app_named.tlf);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
      if (tlf_kind != (size_t) sb_count(type->app_named.types)) {
        err->tag = OO_ERR_WRONG_NUMBER_OF_TYPE_ARGS;
        err->wrong_number_of_type_args = type;
        return;
      }

      count = sb_count(type->app_named.types);
      for (size_t i = 0; i < count; i++) {
        if (kind_arity(&type->app_named.types[i]) != 0) {
          err->tag = OO_ERR_HIGHER_ORDER_TYPE_ARG;
          err->higher_order_type_arg = &type->app_anon.args[i];
          return;
        }

        assert(type->app_named.tlf.binding.type->type.tag == TYPE_GENERIC);
        if (!str_eq(type->app_named.tlf.binding.type->type.generic.args[i].str, type->app_named.sids[i].str)) {
          err->tag = OO_ERR_NAMED_TYPE_APP_SID;
          err->named_type_app_sid = &type->app_named.sids[i];
          return;
        }
      }
      break;
    case TYPE_GENERIC:
      type_kind_checking(cx, err, type->generic.inner);
      break;
    case TYPE_SUM:
      count = sb_count(type->sum.summands);
      for (size_t i = 0; i < count; i++) {
        summand_kind_checking(cx, err, &type->sum.summands[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
}

static void summand_kind_checking(OoContext *cx, OoError *err, AsgSummand *summand) {
  size_t count;
  switch (summand->tag) {
    case SUMMAND_ANON:
      count = sb_count(summand->anon);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &summand->anon[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case SUMMAND_NAMED:
      count = sb_count(summand->named.inners);
      for (size_t i = 0; i < count; i++) {
        type_kind_checking(cx, err, &summand->named.inners[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
}

static void exp_kind_checking(OoContext *cx, OoError *err, AsgExp *exp) {
  size_t count;
  switch (exp->tag) {
    case EXP_MACRO:
      abort(); // macros are evaluated before binding resolution
      break;
    case EXP_ID:
    case EXP_LITERAL:
      // noop
      break;
    case EXP_REF:
      exp_kind_checking(cx, err, exp->ref);
      break;
    case EXP_REF_MUT:
      exp_kind_checking(cx, err, exp->ref_mut);
      break;
    case EXP_DEREF:
      exp_kind_checking(cx, err, exp->deref);
      break;
    case EXP_DEREF_MUT:
      exp_kind_checking(cx, err, exp->deref_mut);
      break;
    case EXP_ARRAY:
      exp_kind_checking(cx, err, exp->array);
      break;
    case EXP_ARRAY_INDEX:
      exp_kind_checking(cx, err, exp->array_index.arr);
      if (err->tag != OO_ERR_NONE) {
        return;
      }
      exp_kind_checking(cx, err, exp->array_index.index);
      break;
    case EXP_PRODUCT_REPEATED:
      exp_kind_checking(cx, err, exp->product_repeated.inner);
      break;
    case EXP_PRODUCT_ANON:
      count = sb_count(exp->product_anon);
      for (size_t i = 0; i < count; i++) {
        exp_kind_checking(cx, err, &exp->product_anon[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_PRODUCT_NAMED:
      count = sb_count(exp->product_named.inners);
      for (size_t i = 0; i < count; i++) {
        exp_kind_checking(cx, err, &exp->product_named.inners[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_PRODUCT_ACCESS_ANON:
      exp_kind_checking(cx, err, exp->product_access_anon.inner);
      break;
    case EXP_PRODUCT_ACCESS_NAMED:
      exp_kind_checking(cx, err, exp->product_access_named.inner);
      break;
    case EXP_FUN_APP_ANON:
      exp_kind_checking(cx, err, exp->fun_app_anon.fun);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->fun_app_anon.args);
      for (size_t i = 0; i < count; i++) {
        exp_kind_checking(cx, err, &exp->fun_app_anon.args[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_FUN_APP_NAMED:
      exp_kind_checking(cx, err, exp->fun_app_named.fun);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->fun_app_named.args);
      for (size_t i = 0; i < count; i++) {
        exp_kind_checking(cx, err, &exp->fun_app_named.args[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_CAST:
      exp_kind_checking(cx, err, exp->cast.inner);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      type_kind_checking(cx, err, exp->cast.type);
      break;
    case EXP_SIZE_OF:
      type_kind_checking(cx, err, exp->size_of);
      break;
    case EXP_ALIGN_OF:
      type_kind_checking(cx, err, exp->align_of);
      break;
    case EXP_NOT:
      exp_kind_checking(cx, err, exp->exp_not);
      break;
    case EXP_NEGATE:
      exp_kind_checking(cx, err, exp->exp_negate);
      break;
    case EXP_WRAPPING_NEGATE:
      exp_kind_checking(cx, err, exp->exp_wrapping_negate);
      break;
    case EXP_BIN_OP:
      exp_kind_checking(cx, err, exp->bin_op.lhs);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      exp_kind_checking(cx, err, exp->bin_op.rhs);
      break;
    case EXP_ASSIGN:
      exp_kind_checking(cx, err, exp->assign.lhs);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      exp_kind_checking(cx, err, exp->assign.rhs);
      break;
    case EXP_VAL:
      pattern_kind_checking(cx, err, &exp->val);
      break;
    case EXP_VAL_ASSIGN:
      pattern_kind_checking(cx, err, &exp->val_assign.lhs);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      exp_kind_checking(cx, err, exp->val_assign.rhs);
      break;
    case EXP_BLOCK:
      block_kind_checking(cx, err, &exp->block);
      break;
    case EXP_IF:
      exp_kind_checking(cx, err, exp->exp_if.cond);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      block_kind_checking(cx, err, &exp->exp_if.if_block);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      block_kind_checking(cx, err, &exp->exp_if.else_block);
      break;
    case EXP_CASE:
      exp_kind_checking(cx, err, exp->exp_case.matcher);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->exp_case.patterns);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &exp->exp_case.patterns[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        block_kind_checking(cx, err, &exp->exp_case.blocks[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_WHILE:
      exp_kind_checking(cx, err, exp->exp_while.cond);
      block_kind_checking(cx, err, &exp->exp_while.block);
      break;
    case EXP_LOOP:
      exp_kind_checking(cx, err, exp->exp_loop.matcher);
      if (err->tag != OO_ERR_NONE) {
        return;
      }

      count = sb_count(exp->exp_case.patterns);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &exp->exp_loop.patterns[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
        block_kind_checking(cx, err, &exp->exp_loop.blocks[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case EXP_RETURN:
      if (exp->exp_return != NULL) {
        exp_kind_checking(cx, err, exp->exp_return);
      }
      break;
    case EXP_BREAK:
      if (exp->exp_break != NULL) {
        exp_kind_checking(cx, err, exp->exp_break);
      }
      break;
    case EXP_GOTO:
    case EXP_LABEL:
      // noop
      break;
  }
}

static void pattern_kind_checking(OoContext *cx, OoError *err, AsgPattern *p) {
  size_t count;
  switch (p->tag) {
    case PATTERN_ID:
      if (p->id.type != NULL) {
        type_kind_checking(cx, err, p->id.type);
      }
      break;
    case PATTERN_BLANK:
    case PATTERN_LITERAL:
      // noop
      break;
    case PATTERN_PTR:
      pattern_kind_checking(cx, err, p->ptr);
      break;
    case PATTERN_PRODUCT_ANON:
      count = sb_count(p->product_anon);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &p->product_anon[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_PRODUCT_NAMED:
      count = sb_count(p->product_named.inners);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &p->product_named.inners[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_SUMMAND_ANON:
      count = sb_count(p->summand_anon.fields);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &p->summand_anon.fields[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
    case PATTERN_SUMMAND_NAMED:
      count = sb_count(p->summand_named.fields);
      for (size_t i = 0; i < count; i++) {
        pattern_kind_checking(cx, err, &p->summand_named.fields[i]);
        if (err->tag != OO_ERR_NONE) {
          return;
        }
      }
      break;
  }
}

static void block_kind_checking(OoContext *cx, OoError *err, AsgBlock *block) {
  for (size_t i = 0; i < (size_t) sb_count(block->exps); i++) {
    exp_kind_checking(cx, err, &block->exps[i]);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}

void oo_cx_kind_checking(OoContext *cx, OoError *err) {
  int count = sb_count(cx->files);
  for (int i = 0; i < count; i++) {
    file_kind_checking(cx, err, cx->files[i]);
    if (err->tag != OO_ERR_NONE) {
      return;
    }
  }
}
