#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "asg.h"
#include "stretchy_buffer.h"
#include "rax.h"

size_t parse_id(const char *src, ParserError *err, AsgId *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;

  err->tag = ERR_NONE;
  data->sids = NULL;
  AsgSid *sid = sb_add(data->sids, 1);
  bool kw = false;
  size_t l;

  switch (t.tt) {
    case KW_MOD:
    case DEP:
    case MAGIC:
      kw = true;
      sid->str.start = data->str.start;
      sid->str.len = t.token_len;
      l = t.len;
      break;
    default:
      l = parse_sid(src, err, sid);
      if (err->tag != ERR_NONE) {
        sb_free(data->sids);
        return l;
      }
      break;
  }

  t = tokenize(src + l);
  l += t.len;
  while (t.tt == SCOPE) {
    sid = sb_add(data->sids, 1);
    l += parse_sid(src + l, err, sid);
    if (err->tag != ERR_NONE) {
      sb_free(data->sids);
      return l;
    }

    t = tokenize(src + l);
    l += t.len;
  }
  l -= t.len;

  if (kw && sb_count(data->sids) == 1) {
    sb_free(data->sids);
    err->tag = ERR_ID;
    return l;
  }

  data->str.len = l - leading_ws;
  return l;
}

void free_inner_id(AsgId data) {
  sb_free(data.sids);
}

size_t parse_sid(const char *src, ParserError *err, AsgSid *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;

  if (t.tt != ID) {
    err->tag = ERR_SID;
    err->tt = t.tt;
    return t.len;
  } else {
    err->tag = ERR_NONE;
  }

  data->str.len = t.token_len;
  return t.len;
}

size_t parse_macro_inv(const char *src, ParserError *err, AsgMacroInv *data) {
  err->tag = ERR_MACRO_INV;

  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;

  size_t l = t.len;
  if (t.tt != DOLLAR) {
    err->tt = t.tt;
    return l;
  }

  AsgSid tmp;
  l += parse_sid(src + l, err, &tmp);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_MACRO_INV;
    return l;
  }
  data->name.start = tmp.str.start;
  data->name.len = tmp.str.len;

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != LPAREN) {
    err->tt = t.tt;
    return l;
  }

  size_t args_start = l;
  size_t nesting = 1;
  while (nesting > 0) {
    t = tokenize(src + l);
    l += t.len;

    if (token_type_error(t.tt)) {
      err->tt = t.tt;
      return l;
    } else if (t.tt == LPAREN) {
      nesting += 1;
    } else if (t.tt == RPAREN) {
      nesting -= 1;
    }
  }

  data->args.start = src + args_start;
  data->args.len = l - (args_start + t.len); // last RPAREN is not part of the args

  err->tag = ERR_NONE;
  data->str.len = l - leading_ws;
  return l;
}

size_t parse_literal(const char *src, ParserError *err, AsgLiteral *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;

  err->tag = ERR_NONE;
  data->str.len = t.token_len;

  if (t.tt == INT) {
    data->tag = LITERAL_INT;
  } else if (t.tt == FLOAT) {
    data->tag = LITERAL_FLOAT;
  } else if (t.tt == STRING) {
    data->tag = LITERAL_STRING;
  } else {
    err->tag = ERR_LITERAL;
  }

  return t.len;
}

size_t parse_bin_op(const char *src, ParserError *err, AsgBinOp *op) {
  size_t l;
  err->tag = ERR_NONE;
  Token t = tokenize(src);
  l = t.len;

  switch (t.tt) {
    case PLUS:
      *op = OP_PLUS;
      return l;
    case MINUS:
      *op = OP_MINUS;
      return l;
    case TIMES:
      *op = OP_TIMES;
      return l;
    case DIV:
      *op = OP_TIMES;
      return l;
    case MOD:
      *op = OP_MOD;
      return l;
    case PIPE:
      *op = OP_OR;
      return l;
    case AMPERSAND:
      *op = OP_AND;
      return l;
    case XOR:
      *op = OP_XOR;
      return l;
    case LAND:
      *op = OP_LAND;
      return l;
    case LOR:
      *op = OP_LOR;
      return l;
    case EQUALS:
      *op = OP_EQ;
      return l;
    case NOTEQUALS:
      *op = OP_NEQ;
      return l;
    case LANGLE:
      t = tokenize(src + l);
      switch (t.tt) {
        case LANGLE:
          *op = OP_SHIFT_L;
          return l + t.len;
        case EQUALS:
          *op = OP_LET;
          return l + t.len;
        default:
          *op = OP_LT;
          return l;
      }
    case RANGLE:
      t = tokenize(src + l);
      switch (t.tt) {
        case RANGLE:
          *op = OP_SHIFT_R;
          return l + t.len;
        case EQUALS:
          *op = OP_GET;
          return l + t.len;
        default:
          *op = OP_GT;
          return l;
      }
    default:
      err->tag = ERR_BIN_OP;
      err->tt = t.tt;
      return t.len;
  }
}

size_t parse_assign_op(const char *src, ParserError *err, AsgAssignOp *op) {
  size_t l;
  err->tag = ERR_NONE;
  Token t = tokenize(src);
  l = t.len;

  switch (t.tt) {
    case EQ:
      *op = ASSIGN_REGULAR;
      return l;
    case PLUS_ASSIGN:
      *op = ASSIGN_PLUS;
      return l;
    case MINUS_ASSIGN:
      *op = ASSIGN_MINUS;
      return l;
    case TIMES_ASSIGN:
      *op = ASSIGN_TIMES;
      return l;
    case DIV_ASSIGN:
      *op = ASSIGN_TIMES;
      return l;
    case MOD_ASSIGN:
      *op = ASSIGN_MOD;
      return l;
    case XOR_ASSIGN:
      *op = ASSIGN_XOR;
      return l;
    case AND_ASSIGN:
      *op = ASSIGN_AND;
      return l;
    case OR_ASSIGN:
      *op = ASSIGN_OR;
      return l;
    case LANGLE:
      t = tokenize(src + l);
      l += t.len;
      if (t.tt != LANGLE) {
        err->tag = ERR_ASSIGN_OP;
        err->tt = t.tt;
        return t.len;
      }

      t = tokenize(src + l);
      l += t.len;

      if (t.tt == EQ) {
        *op = ASSIGN_SHIFT_L;
        return l;
      } else {
        err->tag = ERR_ASSIGN_OP;
        err->tt = t.tt;
        return t.len;
      }
    case RANGLE:
      t = tokenize(src + l);
      l += t.len;
      if (t.tt != RANGLE) {
        err->tag = ERR_ASSIGN_OP;
        err->tt = t.tt;
        return t.len;
      }

      t = tokenize(src + l);
      l += t.len;

      if (t.tt == EQ) {
        *op = ASSIGN_SHIFT_R;
        return l;
      } else {
        err->tag = ERR_ASSIGN_OP;
        err->tt = t.tt;
        return t.len;
      }
    default:
      err->tag = ERR_ASSIGN_OP;
      err->tt = t.tt;
      return t.len;
  }
}

size_t parse_size_of(const char *src, ParserError *err, AsgType *data) {
  err->tag = ERR_NONE;
  size_t l;
  Token t;

  t = tokenize(src);
  l = t.len;
  if (t.tt != SIZEOF) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != LPAREN) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  l += parse_type(src + l, err, data);
  if (err->tag != ERR_NONE) {
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != RPAREN) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  return l;
}

size_t parse_align_of(const char *src, ParserError *err, AsgType *data) {
  err->tag = ERR_NONE;
  size_t l;
  Token t;

  t = tokenize(src);
  l = t.len;
  if (t.tt != ALIGNOF) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != LPAREN) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  l += parse_type(src + l, err, data);
  if (err->tag != ERR_NONE) {
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != RPAREN) {
    err->tag = ERR_TYPE;
    err->tt = t.tt;
    return l;
  }

  return l;
}

size_t parse_repeat(const char *src, ParserError *err, AsgRepeat *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = 0;

  if (t.tt == INT) {
    l += t.len;
    data->str.len = t.token_len;
    data->tag = REPEAT_INT;
  } else if (t.tt == DOLLAR) {
    l += parse_macro_inv(src, err, &(data->macro));
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->str.len = l - leading_ws;
    data->tag = REPEAT_MACRO;
  } else if (t.tt == SIZEOF) {
    l += parse_size_of(src, err, data->size_of);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->str.len = l - leading_ws;
    data->tag = REPEAT_SIZE_OF;
  } else if (t.tt == ALIGNOF) {
    l += parse_align_of(src, err, data->align_of);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->str.len = l - leading_ws;
    data->tag = REPEAT_ALIGN_OF;
  } else {
    err->tag = ERR_REPEAT;
    err->tt = t.tt;
    return l;
  }

  size_t bin_len = parse_bin_op(src + l, err, &(data->bin_op.op));
  if (err->tag != ERR_NONE) {
    err->tag = ERR_NONE;
    return l;
  }

  if (data->bin_op.op == OP_LAND || data->bin_op.op == OP_LOR ||
      data->bin_op.op == OP_EQ || data->bin_op.op == OP_NEQ ||
      data->bin_op.op == OP_GT || data->bin_op.op == OP_GET ||
      data->bin_op.op == OP_LT || data->bin_op.op == OP_GET) {
    return l;
  }

  l += bin_len;

  AsgRepeat *lhs = malloc(sizeof(AsgRepeat));
  memcpy(lhs, data, sizeof(AsgRepeat));
  AsgRepeat *rhs = malloc(sizeof(AsgRepeat));
  data->tag = REPEAT_BIN_OP;
  data->bin_op.lhs = lhs;
  data->bin_op.rhs = rhs;

  l += parse_repeat(src + l, err, rhs);
  if (err->tag != ERR_NONE) {
    free(lhs);
    free(rhs);
    return l;
  }

  data->str.len = l - leading_ws;
  return l;
}

void free_inner_repeat(AsgRepeat data) {
  if (data.tag == REPEAT_BIN_OP) {
    free_inner_repeat(*(data.bin_op.lhs));
    free(data.bin_op.lhs);
    free_inner_repeat(*(data.bin_op.rhs));
    free(data.bin_op.rhs);
  }
}

void free_sb_types(AsgType *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_type(sb[i]);
  }
  sb_free(sb);
}

void free_sb_summands(AsgSummand *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    switch (sb[i].tag) {
      case SUMMAND_ANON:
        free_sb_types(sb[i].anon);
        break;
      case SUMMAND_NAMED:
        free_sb_types(sb[i].named.inners);
        sb_free(sb[i].named.sids);
        break;
    }
  }
  sb_free(sb);
}

size_t parse_type(const char *src, ParserError *err, AsgType *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = 0;

  AsgId id;
  bool pub = false;
  AsgSummand *summands = NULL;
  switch (t.tt) {
    case ID:
      l += parse_id(src, err, &id);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
      }

      t = tokenize(src + l);
      if (t.tt == LANGLE) {
        // type application
        l += t.len;
        t = tokenize(src + l);

        if (t.tt == ID) {
          // named app iff the next token is EQUALS
          Token t2 = tokenize(src + l + t.len);
          if (t2.tt == EQ) {
            // named app
            AsgSid *sids = NULL;
            AsgType *types = NULL;

            AsgSid *sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            l += t2.len;
            AsgType *type;
            type = sb_add(types, 1);
            l += parse_type(src + l, err, type);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_types(types);
              return l;
            }
            t = tokenize(src + l);
            l += t.len;

            while (t.tt == COMMA) {
              sid = sb_add(sids, 1);
              l += parse_sid(src + l, err, sid);
              if (err->tag != ERR_NONE) {
                sb_free(sids);
                free_sb_types(types);
                err->tag = ERR_TYPE;
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
              if (t.tt != EQ) {
                sb_free(sids);
                free_sb_types(types);
                err->tag = ERR_TYPE;
                err->tt = t.tt;
                return l;
              }

              type = sb_add(types, 1);
              l += parse_type(src + l, err, type);
              if (err->tag != ERR_NONE) {
                sb_free(sids);
                free_sb_types(types);
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
            }

            if (t.tt == RANGLE) {
              data->tag = TYPE_APP_NAMED;
              data->str.len = l - leading_ws;
              data->app_named.tlf = id;
              data->app_named.types = types;
              data->app_named.sids = sids;
              return l;
            } else {
              err->tag = ERR_TYPE;
              err->tt = t.tt;
              sb_free(sids);
              free_sb_types(types);
              return l;
            }
          }
        }

        AsgType *inners = NULL;
        AsgType *inner = sb_add(inners, 1);
        l += parse_type(src + l, err, inner);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_TYPE;
          free_sb_types(inners);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != COMMA && t.tt != RANGLE) {
          err->tag = ERR_TYPE;
          err->tt = t.tt;
          free_sb_types(inners);
          return l;
        } else {
          // anon app
          while (t.tt == COMMA) {
            inner = sb_add(inners, 1);
            l += parse_type(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              free_sb_types(inners);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }
          if (t.tt != RANGLE) {
            free_sb_types(inners);
            err->tag = ERR_TYPE;
            err->tt = t.tt;
            return l;
          }

          data->tag = TYPE_APP_ANON;
          data->str.len = l - leading_ws;
          data->app_anon.tlf = id;
          data->app_anon.args = inners;
          return l;
        }
      } else {
        // not a type application, just an id
        data->tag = TYPE_ID;
        data->id = id;
        data->str.len = data->id.str.len;
        return l;
      }
    case DOLLAR:
      l += parse_macro_inv(src, err, &data->macro);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
      }
      data->tag = TYPE_MACRO;
      data->str.len = data->macro.str.len;
      return l;
    case AT:
      l += t.len;
      AsgType *inner_ptr = malloc(sizeof(AsgType));
      l += parse_type(src + l, err, inner_ptr);
      if (err->tag != ERR_NONE) {
        free(inner_ptr);
      }
      data->tag = TYPE_PTR;
      data->str.len = l - leading_ws;
      data->ptr = inner_ptr;
      return l;
    case TILDE:
      l += t.len;
      AsgType *inner_ptr_mut = malloc(sizeof(AsgType));
      l += parse_type(src + l, err, inner_ptr_mut);
      if (err->tag != ERR_NONE) {
        free(inner_ptr_mut);
      }
      data->tag = TYPE_PTR_MUT;
      data->str.len = l - leading_ws;
      data->ptr_mut = inner_ptr_mut;
      return l;
    case LBRACKET:
      l += t.len;
      AsgType *inner_array = malloc(sizeof(AsgType));

      l += parse_type(src + l, err, inner_array);
      if (err->tag != ERR_NONE) {
        free(inner_array);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != RBRACKET) {
        err->tag = ERR_TYPE;
        err->tt = t.tt;
        free(inner_array);
        return l;
      }

      data->tag = TYPE_ARRAY;
      data->str.len = l - leading_ws;
      data->array = inner_array;
      return l;
    case LANGLE:
      l += t.len;

      AsgSid *args = NULL;
      AsgSid *arg = sb_add(args, 1);
      l += parse_sid(src + l, err, arg);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
        sb_free(args);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != COMMA && t.tt != RANGLE) {
        err->tag = ERR_TYPE;
        err->tt = t.tt;
        sb_free(args);
        return l;
      } else {
        while (t.tt == COMMA) {
          arg = sb_add(args, 1);
          l += parse_sid(src + l, err, arg);
          if (err->tag != ERR_NONE) {
            sb_free(args);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RANGLE) {
          sb_free(args);
          err->tag = ERR_TYPE;
          err->tt = t.tt;
          return l;
        }

        t = tokenize(src + l);
        if (t.tt != FAT_ARROW) {
          sb_free(args);
          err->tag = ERR_TYPE;
          err->tt = t.tt;
          return l;
        }
        l += t.len;

        AsgType *inner = malloc(sizeof(AsgType));
        l += parse_type(src + l, err, inner);
        if (err->tag != ERR_NONE) {
          sb_free(args);
          free(inner);
          return l;
        }

        data->tag = TYPE_GENERIC;
        data->str.len = l - leading_ws;
        data->generic.args = args;
        data->generic.inner = inner;
        return l;
      }
    case LPAREN:
      // handles empty product, repeated product, anon fun, anon product,
      // named fun, named product
      l += t.len;
      t = tokenize(src + l);
      if (t.tt == RPAREN) {
        // empty (anon) product or fun without args
        l += t.len;

        t = tokenize(src + l);
        if (t.tt == ARROW) {
          l += t.len;
          AsgType *ret = malloc(sizeof(AsgType));

          l += parse_type(src + l, err, ret);
          if (err->tag != ERR_NONE) {
            err->tag = ERR_TYPE;
            return l;
          }
          data->tag = TYPE_FUN_ANON;
          data->str.len = l - leading_ws;
          data->fun_anon.args = NULL;
          data->fun_anon.ret = ret;
          return l;
        } else {
          data->str.len = l - leading_ws;
          data->tag = TYPE_PRODUCT_ANON;
          data->product_anon = NULL;
          return l;
        }
      } else if (t.tt == ID) {
        // named fun, named product iff the next token is COLON
        Token t2 = tokenize(src + l + t.len);
        if (t2.tt == COLON) {
          // named fun, named product
          AsgSid *sids = NULL;
          AsgType *types = NULL;

          AsgSid *sid = sb_add(sids, 1);
          l += parse_sid(src + l, err, sid);
          l += t2.len;
          AsgType *type;
          type = sb_add(types, 1);
          l += parse_type(src + l, err, type);
          if (err->tag != ERR_NONE) {
            sb_free(sids);
            free_sb_types(types);
            return l;
          }
          t = tokenize(src + l);
          l += t.len;

          while (t.tt == COMMA) {
            sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_types(types);
              err->tag = ERR_TYPE;
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
            if (t.tt != COLON) {
              sb_free(sids);
              free_sb_types(types);
              err->tag = ERR_TYPE;
              err->tt = t.tt;
              return l;
            }

            type = sb_add(types, 1);
            l += parse_type(src + l, err, type);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_types(types);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }

          t = tokenize(src + l);
          if (t.tt == ARROW) {
            // named fun
            l += t.len;
            AsgType *ret = malloc(sizeof(AsgType));

            l += parse_type(src + l, err, ret);
            if (err->tag != ERR_NONE) {
              err->tag = ERR_TYPE;
              sb_free(sids);
              free_sb_types(types);
              return l;
            }
            data->tag = TYPE_FUN_NAMED;
            data->str.len = l - leading_ws;
            data->fun_named.arg_types = types;
            data->fun_named.arg_sids = sids;
            data->fun_named.ret = ret;
            return l;
          }
          data->tag = TYPE_PRODUCT_NAMED;
          data->str.len = l - leading_ws;
          data->product_named.types = types;
          data->product_named.sids = sids;
          return l;
        }
      }
      // repeated product, anon fun, anon product
      AsgType *inners = NULL;
      AsgType *inner = sb_add(inners, 1);
      l += parse_type(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
        free_sb_types(inners);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt == SEMI) {
        l += parse_repeat(src + l, err, &data->product_repeated.repeat);
        if (err->tag != ERR_NONE) {
          free_sb_types(inners);
          err->tag = ERR_TYPE;
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != RPAREN) {
          free_sb_types(inners);
          err->tag = ERR_TYPE;
          err->tt = t.tt;
          return l;
        }

        data->tag = TYPE_PRODUCT_REPEATED;
        data->str.len = l - leading_ws;
        AsgType *inner = malloc(sizeof(AsgType));
        memcpy(inner, inners, sizeof(AsgType));
        data->product_repeated.inner = inner;
        sb_free(inners);
        return l;
      } else if (t.tt != COMMA && t.tt != RPAREN) {
        err->tag = ERR_TYPE;
        err->tt = t.tt;
        free_sb_types(inners);
        return l;
      } else {
        // anon fun, anon product
        while (t.tt == COMMA) {
          inner = sb_add(inners, 1);
          l += parse_type(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            free_sb_types(inners);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RPAREN) {
          free_sb_types(inners);
          err->tag = ERR_TYPE;
          err->tt = t.tt;
          return l;
        }

        t = tokenize(src + l);
        if (t.tt == ARROW) {
          // anon fun
          l += t.len;
          AsgType *ret = malloc(sizeof(AsgType));

          l += parse_type(src + l, err, ret);
          if (err->tag != ERR_NONE) {
            err->tag = ERR_TYPE;
            free_sb_types(inners);
            return l;
          }
          data->tag = TYPE_FUN_ANON;
          data->str.len = l - leading_ws;
          data->fun_anon.args = inners;
          data->fun_anon.ret = ret;
          return l;
        } else {
          data->tag = TYPE_PRODUCT_ANON;
          data->str.len = l - leading_ws;
          data->product_anon = inners;
          return l;
        }
      }
    case PUB:
      pub = true;
      l += t.len;
      t = tokenize(src + l);
      if (t.tt != PIPE) {
        err->tag = ERR_TYPE;
        err->tt = t.tt;
        return l + t.len;
      }
      __attribute__((fallthrough));
    case PIPE:
      while (t.tt == PIPE) {
        AsgSummand *summand = sb_add(summands, 1);
        l += parse_summand(src + l, err, summand);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_TYPE;
          free_sb_summands(summands);
          return l;
        }

        t = tokenize(src + l);
      }

      data->tag = TYPE_SUM;
      data->str.len = l - leading_ws;
      data->sum.pub = pub;
      data->sum.summands = summands;
      return l;
    default:
      err->tag = ERR_TYPE;
      err->tt = t.tt;
      data->str.len = t.len;
      return t.len;
  }
}

size_t parse_summand(const char *src, ParserError *err, AsgSummand *data) {
  size_t l = 0;
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  l += t.len;
  if (t.tt != PIPE) {
    err->tag = ERR_SUMMAND;
    err->tt = t.tt;
    return l;
  }

  l += parse_sid(src + l, err, &data->sid);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_SUMMAND;
    return l;
  }

  t = tokenize(src + l);
  if (t.tt == LPAREN) {
    l += t.len;
    t = tokenize(src + l);

    if (t.tt == ID) {
      // named summand iff the next token is COLON
      Token t2 = tokenize(src + l + t.len);
      if (t2.tt == COLON) {
        // named summand
        AsgSid *sids = NULL;
        AsgType *types = NULL;

        AsgSid *sid = sb_add(sids, 1);
        l += parse_sid(src + l, err, sid);
        l += t2.len;
        AsgType *type;
        type = sb_add(types, 1);
        l += parse_type(src + l, err, type);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_SUMMAND;
          sb_free(sids);
          free_sb_types(types);
          return l;
        }
        t = tokenize(src + l);
        l += t.len;

        while (t.tt == COMMA) {
          sid = sb_add(sids, 1);
          l += parse_sid(src + l, err, sid);
          if (err->tag != ERR_NONE) {
            sb_free(sids);
            free_sb_types(types);
            err->tag = ERR_SUMMAND;
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
          if (t.tt != COLON) {
            sb_free(sids);
            free_sb_types(types);
            err->tag = ERR_SUMMAND;
            err->tt = t.tt;
            return l;
          }

          type = sb_add(types, 1);
          l += parse_type(src + l, err, type);
          if (err->tag != ERR_NONE) {
            sb_free(sids);
            free_sb_types(types);
            err->tag = ERR_SUMMAND;
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }

        if (t.tt == RPAREN) {
          data->tag = SUMMAND_NAMED;
          data->str.len = l - leading_ws;
          data->named.inners = types;
          data->named.sids = sids;
          return l;
        } else {
          err->tag = ERR_SUMMAND;
          err->tt = t.tt;
          sb_free(sids);
          free_sb_types(types);
          return l;
        }
      }
    }

    AsgType *inners = NULL;
    AsgType *inner = sb_add(inners, 1);
    l += parse_type(src + l, err, inner);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_SUMMAND;
      free_sb_types(inners);
      return l;
    }

    t = tokenize(src + l);
    l += t.len;
    if (t.tt != COMMA && t.tt != RPAREN) {
      err->tag = ERR_SUMMAND;
      err->tt = t.tt;
      free_sb_types(inners);
      return l;
    } else {
      // anon summand
      while (t.tt == COMMA) {
        inner = sb_add(inners, 1);
        l += parse_type(src + l, err, inner);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_SUMMAND;
          free_sb_types(inners);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
      }
      if (t.tt != RPAREN) {
        free_sb_types(inners);
        err->tag = ERR_SUMMAND;
        err->tt = t.tt;
        return l;
      }

      data->tag = SUMMAND_ANON;
      data->str.len = l - leading_ws;
      data->anon = inners;
      return l;
    }
  } else {
    // Identifier without parens (empty anon)
    data->str.len = l - leading_ws;
    data->tag = SUMMAND_ANON;
    data->anon = NULL;
    return l;
  }
}

void free_inner_type(AsgType data) {
  switch (data.tag) {
    case TYPE_ID:
      free_inner_id(data.id);
      break;
    case TYPE_PTR:
      free_inner_type(*data.ptr);
      free(data.ptr);
      break;
    case TYPE_PTR_MUT:
      free_inner_type(*data.ptr_mut);
      free(data.ptr_mut);
      break;
    case TYPE_ARRAY:
      free_inner_type(*data.array);
      free(data.array);
      break;
    case TYPE_PRODUCT_REPEATED:
      free_inner_type(*data.product_repeated.inner);
      free(data.product_repeated.inner);
      free_inner_repeat(data.product_repeated.repeat);
    break;
    case TYPE_PRODUCT_ANON:
      free_sb_types(data.product_anon);
      break;
    case TYPE_PRODUCT_NAMED:
      free_sb_types(data.product_named.types);
      sb_free(data.product_named.sids);
      break;
    case TYPE_FUN_ANON:
      free_sb_types(data.fun_anon.args);
      free_inner_type(*data.fun_anon.ret);
      free(data.fun_anon.ret);
      break;
    case TYPE_FUN_NAMED:
      free_sb_types(data.fun_named.arg_types);
      sb_free(data.fun_named.arg_sids);
      free_inner_type(*data.fun_named.ret);
      free(data.fun_named.ret);
      break;
    case TYPE_APP_ANON:
      free_inner_id(data.app_anon.tlf);
      free_sb_types(data.app_anon.args);
      break;
    case TYPE_APP_NAMED:
      free_inner_id(data.app_named.tlf);
      free_sb_types(data.app_named.types);
      sb_free(data.app_named.sids);
      break;
    case TYPE_GENERIC:
      free_inner_type(*data.generic.inner);
      free(data.generic.inner);
      sb_free(data.generic.args);
      break;
    case TYPE_SUM:
      free_sb_summands(data.sum.summands);
      break;
    default:
      return;
  }
}

void free_sb_patterns(AsgPattern *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_pattern(sb[i]);
  }
  sb_free(sb);
}

size_t parse_pattern(const char *src, ParserError *err, AsgPattern *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = 0;

  bool mut = false;
  AsgType *type = NULL;
  AsgId id;
  switch (t.tt) {
    case UNDERSCORE:
      l += t.len;
      data->tag = PATTERN_BLANK;
      data->str.len = l - leading_ws;
      return l;
    case MUT:
      mut = true;
      l += t.len;
      t = tokenize(src + l);
      if (t.tt != ID) {
        err->tag = ERR_PATTERN;
        err->tt = t.tt;
        return l + t.len;
      }
      __attribute__((fallthrough));
    case ID:
      l += parse_sid(src + l, err, &data->id.sid);

      t = tokenize(src + l);
      if (t.tt == COLON) {
        l += t.len;
        type = malloc(sizeof(AsgType));
        l += parse_type(src + l, err, type);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_PATTERN;
          free(type);
          return l;
        }
      }

      data->tag = PATTERN_ID;
      data->str.len = l - leading_ws;
      data->id.mut = mut;
      data->id.type = type;
      return l;
    case INT:
    case FLOAT:
    case STRING:
      l += parse_literal(src + l, err, &data->lit);
      data->str.len = l - leading_ws;
      data->tag = PATTERN_LITERAL;
      return l;
    case AT:
      l += t.len;
      AsgPattern *inner_ptr = malloc(sizeof(AsgPattern));
      l += parse_pattern(src + l, err, inner_ptr);
      if (err->tag != ERR_NONE) {
        free(inner_ptr);
      }
      data->tag = PATTERN_PTR;
      data->str.len = l - leading_ws;
      data->ptr = inner_ptr;
      return l;
    case LPAREN:
      l += t.len;

      t = tokenize(src + l);
      if (t.tt == RPAREN) {
        l += t.len;
        data->tag = PATTERN_PRODUCT_ANON;
        data->str.len = l - leading_ws;
        data->product_anon = NULL;
        return l;
      }

      if (t.tt == ID) {
        // named product iff the next token is EQUALS
        Token t2 = tokenize(src + l + t.len);
        if (t2.tt == EQ) {
          // named product
          AsgSid *sids = NULL;
          AsgPattern *inners = NULL;

          AsgSid *sid = sb_add(sids, 1);
          l += parse_sid(src + l, err, sid);
          l += t2.len;
          AsgPattern *inner;
          inner = sb_add(inners, 1);
          l += parse_pattern(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            sb_free(sids);
            free_sb_patterns(inners);
            return l;
          }
          t = tokenize(src + l);
          l += t.len;

          while (t.tt == COMMA) {
            sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_patterns(inners);
              err->tag = ERR_PATTERN;
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
            if (t.tt != EQ) {
              sb_free(sids);
              free_sb_patterns(inners);
              err->tag = ERR_PATTERN;
              err->tt = t.tt;
              return l;
            }

            inner = sb_add(inners, 1);
            l += parse_pattern(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_patterns(inners);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }

          if (t.tt == RPAREN) {
            data->tag = PATTERN_PRODUCT_NAMED;
            data->str.len = l - leading_ws;
            data->product_named.inners = inners;
            data->product_named.sids = sids;
            return l;
          } else {
            err->tag = ERR_PATTERN;
            err->tt = t.tt;
            sb_free(sids);
            free_sb_patterns(inners);
            return l;
          }
        }
      }

      AsgPattern *inners = NULL;
      AsgPattern *inner = sb_add(inners, 1);
      l += parse_pattern(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_PATTERN;
        free_sb_patterns(inners);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != COMMA && t.tt != RPAREN) {
        err->tag = ERR_PATTERN;
        err->tt = t.tt;
        free_sb_patterns(inners);
        return l;
      } else {
        // anon product
        while (t.tt == COMMA) {
          inner = sb_add(inners, 1);
          l += parse_pattern(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            free_sb_patterns(inners);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RPAREN) {
          free_sb_patterns(inners);
          err->tag = ERR_PATTERN;
          err->tt = t.tt;
          return l;
        }

        data->tag = PATTERN_PRODUCT_ANON;
        data->str.len = l - leading_ws;
        data->product_anon = inners;
        return l;
      }
    case PIPE:
      l += t.len;

      l += parse_id(src + l, err, &id);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_PATTERN;
        return l;
      }

      t = tokenize(src + l);
      if (t.tt == LPAREN) {
        l += t.len;
        t = tokenize(src + l);

        if (t.tt == ID) {
          // named summand iff the next token is EQ
          Token t2 = tokenize(src + l + t.len);
          if (t2.tt == EQ) {
            // named summand
            AsgSid *sids = NULL;
            AsgPattern *inners = NULL;

            AsgSid *sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            l += t2.len;
            AsgPattern *inner;
            inner = sb_add(inners, 1);
            l += parse_pattern(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              err->tag = ERR_PATTERN;
              sb_free(sids);
              free_sb_patterns(inners);
              return l;
            }
            t = tokenize(src + l);
            l += t.len;

            while (t.tt == COMMA) {
              sid = sb_add(sids, 1);
              l += parse_sid(src + l, err, sid);
              if (err->tag != ERR_NONE) {
                err->tag = ERR_PATTERN;
                sb_free(sids);
                free_sb_patterns(inners);
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
              if (t.tt != EQ) {
                err->tag = ERR_PATTERN;
                sb_free(sids);
                free_sb_patterns(inners);
                err->tt = t.tt;
                return l;
              }

              inner = sb_add(inners, 1);
              l += parse_pattern(src + l, err, inner);
              if (err->tag != ERR_NONE) {
                err->tag = ERR_PATTERN;
                sb_free(sids);
                free_sb_patterns(inners);
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
            }

            if (t.tt == RPAREN) {
              data->tag = PATTERN_SUMMAND_NAMED;
              data->str.len = l - leading_ws;
              data->summand_named.id = id;
              data->summand_named.fields = inners;
              data->summand_named.sids = sids;
              return l;
            } else {
              err->tt = t.tt;
              err->tag = ERR_PATTERN;
              sb_free(sids);
              free_sb_patterns(inners);
              return l;
            }
          }
        }

        AsgPattern *inners = NULL;
        AsgPattern *inner = sb_add(inners, 1);
        l += parse_pattern(src + l, err, inner);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_PATTERN;
          free_sb_patterns(inners);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != COMMA && t.tt != RPAREN) {
          err->tt = t.tt;
          err->tag = ERR_PATTERN;
          free_sb_patterns(inners);
          return l;
        } else {
          // anon summand
          while (t.tt == COMMA) {
            inner = sb_add(inners, 1);
            l += parse_pattern(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              err->tag = ERR_PATTERN;
              free_sb_patterns(inners);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }
          if (t.tt != RPAREN) {
            err->tag = ERR_PATTERN;
            free_sb_patterns(inners);
            err->tt = t.tt;
            return l;
          }

          data->tag = PATTERN_SUMMAND_ANON;
          data->str.len = l - leading_ws;
          data->summand_anon.id = id;
          data->summand_anon.fields = inners;
          return l;
        }
      } else {
        // Identifier without parens (empty anon)
        data->str.len = l - leading_ws;
        data->tag = PATTERN_SUMMAND_ANON;
        data->summand_anon.id = id;
        data->summand_anon.fields = NULL;
        return l;
      }
    default:
      err->tag = ERR_TYPE;
      err->tt = t.tt;
      data->str.len = t.len;
      return t.len;
  }
}

void free_inner_pattern(AsgPattern data) {
  switch (data.tag) {
    case PATTERN_ID:
      if (data.id.type) {
        free_inner_type(*data.id.type);
        free(data.id.type);
      }
      break;
    case PATTERN_PTR:
      free_inner_pattern(*data.ptr);
      free(data.ptr);
      break;
    case PATTERN_PRODUCT_ANON:
      free_sb_patterns(data.product_anon);
      break;
    case PATTERN_PRODUCT_NAMED:
      free_sb_patterns(data.product_named.inners);
      sb_free(data.product_named.sids);
      break;
    case PATTERN_SUMMAND_ANON:
      free_inner_id(data.summand_anon.id);
      free_sb_patterns(data.summand_anon.fields);
      break;
    case PATTERN_SUMMAND_NAMED:
      free_inner_id(data.summand_named.id);
      free_sb_patterns(data.summand_named.fields);
      sb_free(data.summand_named.sids);
      break;
    default:
      return;
  }
}

void free_sb_meta(AsgMeta *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_meta(sb[i]);
  }
  sb_free(sb);
}

void free_sb_sb_meta(AsgMeta **sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_sb_meta(sb[i]);
  }
  sb_free(sb);
}

size_t parse_meta(const char *src, ParserError *err, AsgMeta *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = t.len;
  if (t.tt != ID) {
    err->tag = ERR_META;
    err->tt = t.tt;
    return l;
  }
  data->name.start = src + leading_ws;
  data->name.len = t.token_len;

  t = tokenize(src + l);
  switch (t.tt) {
    case EQ:
      l += t.len;
      l += parse_literal(src + l, err, &data->unary);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_META;
        return l;
      }

      data->tag = META_UNARY;
      data->str.len = l - leading_ws;
      return l;
    case LPAREN:
      l += t.len;

      data->nested = NULL;
      AsgMeta *inner = sb_add(data->nested, 1);
      l += parse_meta(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_META;
        free_sb_meta(data->nested);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != COMMA && t.tt != RPAREN) {
        err->tag = ERR_META;
        free_sb_meta(data->nested);
        err->tt = t.tt;
        return l;
      } else {
        while (t.tt == COMMA) {
          inner = sb_add(data->nested, 1);
          l += parse_meta(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            free_sb_meta(data->nested);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RPAREN) {
          err->tag = ERR_EXP;
          free_sb_meta(data->nested);
          err->tt = t.tt;
          return l;
        }

        data->tag = META_NESTED;
        data->str.len = l - leading_ws;
        return l;
      }
    default:
      data->tag = META_NULLARY;
      data->str.len = l - leading_ws;
      return l;
  }
}

void free_inner_meta(AsgMeta data) {
  switch (data.tag) {
    case META_NESTED:
      free_sb_meta(data.nested);
      break;
    default:
      return;
  }
}

size_t parse_attr(const char *src, ParserError *err, AsgMeta *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = t.len;

  if (t.tt != BEGIN_ATTRIBUTE) {
    err->tag = ERR_ATTR;
    err->tt = t.tt;
    return l;
  }

  l += parse_meta(src + l, err, data);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_ATTR;
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  if (t.tt != RBRACKET) {
    err->tag = ERR_ATTR;
    err->tt = t.tt;
    return l;
  }

  data->str.len = l - leading_ws;
  return l;
}

size_t parse_attrs(const char *src, ParserError *err, AsgMeta **attrs /* ptr to sb */) {
  size_t l = 0;
  Token t = tokenize(src + l);
  while (t.tt == BEGIN_ATTRIBUTE) {
    AsgMeta *attr = sb_add(*attrs, 1);
    l += parse_attr(src + l, err, attr);
    if (err->tag != ERR_NONE) {
      return l;
    }
    t = tokenize(src + l);
  }
  return l;
}

void free_sb_exps(AsgExp *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_exp(sb[i]);
  }
  sb_free(sb);
}

size_t parse_block(const char *src, ParserError *err, AsgBlock *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = t.len;

  if (t.tt != LBRACE) {
    err->tag = ERR_BLOCK;
    err->tt = t.tt;
    return l;
  }

  AsgExp *exps = NULL;
  AsgMeta **all_attrs = NULL; // sb of sbs

  t = tokenize(src + l);

  if (t.tt == RBRACE) {
    l += t.len;
    data->str.len = l - leading_ws;
    data->exps = exps;
    data->attrs = all_attrs;
    return l;
  }

  AsgMeta **attrs = sb_add(all_attrs, 1); // ptr to an sb
  *attrs = NULL;

  l += parse_attrs(src + l, err, attrs);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_BLOCK;
    free_sb_exps(exps);
    free_sb_sb_meta(all_attrs);
    return l;
  }

  AsgExp *exp = sb_add(exps, 1);
  l += parse_exp(src + l, err, exp);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_BLOCK;
    free_sb_exps(exps);
    free_sb_sb_meta(all_attrs);
    return l;
  }

  t = tokenize(src + l);
  l += t.len;

  while (t.tt == SEMI) {
    AsgMeta **attrs = sb_add(all_attrs, 1); // ptr to an sb
    *attrs = NULL;
    l += parse_attrs(src + l, err, attrs);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_BLOCK;
      free_sb_exps(exps);
      free_sb_sb_meta(all_attrs);
      return l;
    }

    AsgExp *exp = sb_add(exps, 1);
    l += parse_exp(src + l, err, exp);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_BLOCK;
      free_sb_exps(exps);
      free_sb_sb_meta(all_attrs);
      return l;
    }

    t = tokenize(src + l);
    l += t.len;
  }

  if (t.tt == RBRACE) {
    data->str.len = l - leading_ws;
    data->exps = exps;
    data->attrs = all_attrs;
    return l;
  } else {
    err->tag = ERR_BLOCK;
    free_sb_exps(exps);
    free_sb_sb_meta(all_attrs);
    return l;
  }
}

void free_inner_block(AsgBlock data) {
  free_sb_exps(data.exps);
  free_sb_sb_meta(data.attrs);
}

void free_sb_blocks(AsgBlock *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_block(sb[i]);
  }
  sb_free(sb);
}

size_t parse_exp_non_left_recursive(const char *src, ParserError *err, AsgExp *data) {
  Token t = tokenize(src);
  size_t leading_ws = t.len - t.token_len;
  data->str.start = src + leading_ws;
  err->tag = ERR_NONE;
  size_t l = 0;

  switch (t.tt) {
    case ID:
      l += parse_id(src, err, &data->id);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
      }
      data->str.len = l - leading_ws;
      data->tag = EXP_ID;
      return l;
    case DOLLAR:
      l += parse_macro_inv(src, err, &data->macro);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
      }
      data->tag = EXP_MACRO;
      data->str.len = data->macro.str.len;
      return l;
    case INT:
    case FLOAT:
    case STRING:
      l += parse_literal(src + l, err, &data->lit);
      data->str.len = l - leading_ws;
      data->tag = EXP_LITERAL;
      return l;
    case AT:
      l += t.len;
      AsgExp *inner_ref = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_ref);
      if (err->tag != ERR_NONE) {
        free(inner_ref);
      }
      data->tag = EXP_REF;
      data->str.len = l - leading_ws;
      data->ref = inner_ref;
      return l;
    case TILDE:
      l += t.len;
      AsgExp *inner_ref_mut = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_ref_mut);
      if (err->tag != ERR_NONE) {
        free(inner_ref_mut);
      }
      data->tag = EXP_REF_MUT;
      data->str.len = l - leading_ws;
      data->ref_mut = inner_ref_mut;
      return l;
    case LBRACE:
      l += parse_block(src + l, err, &data->block);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
      }
      data->tag = EXP_BLOCK;
      data->str.len = l - leading_ws;
      return l;
    case LBRACKET:
      l += t.len;
      AsgExp *inner_array = malloc(sizeof(AsgExp));

      l += parse_exp(src + l, err, inner_array);
      if (err->tag != ERR_NONE) {
        free(inner_array);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != RBRACKET) {
        err->tag = ERR_EXP;
        err->tt = t.tt;
        free(inner_array);
        return l;
      }

      data->tag = EXP_ARRAY;
      data->str.len = l - leading_ws;
      data->array = inner_array;
      return l;
    case LPAREN:
      // handles empty product, repeated product, anon product, named product
      l += t.len;
      t = tokenize(src + l);
      if (t.tt == RPAREN) {
        // empty (anon) product
        l += t.len;

        data->str.len = l - leading_ws;
        data->tag = EXP_PRODUCT_ANON;
        data->product_anon = NULL;
        return l;
      } else if (t.tt == ID) {
        // named product iff the next token is EQ
        Token t2 = tokenize(src + l + t.len);
        if (t2.tt == EQ) {
          // named fun, named product
          AsgSid *sids = NULL;
          AsgExp *inners = NULL;

          AsgSid *sid = sb_add(sids, 1);
          l += parse_sid(src + l, err, sid);
          l += t2.len;
          AsgExp *inner;
          inner = sb_add(inners, 1);
          l += parse_exp(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            sb_free(sids);
            free_sb_exps(inners);
            return l;
          }
          t = tokenize(src + l);
          l += t.len;

          while (t.tt == COMMA) {
            sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_exps(inners);
              err->tag = ERR_EXP;
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
            if (t.tt != EQ) {
              sb_free(sids);
              free_sb_exps(inners);
              err->tag = ERR_EXP;
              err->tt = t.tt;
              return l;
            }

            inner = sb_add(inners, 1);
            l += parse_exp(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_exps(inners);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }

          data->tag = EXP_PRODUCT_NAMED;
          data->str.len = l - leading_ws;
          data->product_named.inners = inners;
          data->product_named.sids = sids;
          return l;
        }
      }
      // repeated product, anon product
      AsgExp *inners = NULL;
      AsgExp *inner = sb_add(inners, 1);
      l += parse_exp(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
        free_sb_exps(inners);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt == SEMI) {
        l += parse_repeat(src + l, err, &data->product_repeated.repeat);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free_sb_exps(inners);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != RPAREN) {
          err->tag = ERR_EXP;
          free_sb_exps(inners);
          err->tt = t.tt;
          return l;
        }

        data->tag = EXP_PRODUCT_REPEATED;
        data->str.len = l - leading_ws;
        AsgExp *inner = malloc(sizeof(AsgExp));
        memcpy(inner, inners, sizeof(AsgExp));
        data->product_repeated.inner = inner;
        sb_free(inners);
        return l;
      } else if (t.tt != COMMA && t.tt != RPAREN) {
        err->tag = ERR_EXP;
        free_sb_exps(inners);
        err->tt = t.tt;
        return l;
      } else {
        // anon product
        while (t.tt == COMMA) {
          inner = sb_add(inners, 1);
          l += parse_exp(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            free_sb_exps(inners);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RPAREN) {
          err->tag = ERR_EXP;
          free_sb_exps(inners);
          err->tt = t.tt;
          return l;
        }

        data->tag = EXP_PRODUCT_ANON;
        data->str.len = l - leading_ws;
        data->product_anon = inners;
        return l;
      }
    case SIZEOF:
      ;
      AsgType *inner_size_of = malloc(sizeof(AsgType));
      l += parse_size_of(src + l, err, inner_size_of);
      if (err->tag != ERR_NONE) {
        free(inner_size_of);
      }
      data->tag = EXP_SIZE_OF;
      data->str.len = l - leading_ws;
      data->size_of = inner_size_of;
      return l;
    case ALIGNOF:
      ;
      AsgType *inner_align_of = malloc(sizeof(AsgType));
      l += parse_align_of(src + l, err, inner_align_of);
      if (err->tag != ERR_NONE) {
        free(inner_align_of);
      }
      data->tag = EXP_ALIGN_OF;
      data->str.len = l - leading_ws;
      data->align_of = inner_align_of;
      return l;
    case NOT:
      l += t.len;
      AsgExp *inner_not = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_not);
      if (err->tag != ERR_NONE) {
        free(inner_not);
      }
      data->tag = EXP_NOT;
      data->str.len = l - leading_ws;
      data->exp_not = inner_not;
      return l;
    case MINUS:
      l += t.len;
      AsgExp *inner_negate = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_negate);
      if (err->tag != ERR_NONE) {
        free(inner_negate);
      }
      data->tag = EXP_NEGATE;
      data->str.len = l - leading_ws;
      data->exp_negate = inner_negate;
      return l;
    case VAL:
      l += t.len;
      l += parse_pattern(src + l, err, &data->val);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
        return l;
      }

      t = tokenize(src + l);
      if (t.tt == EQ) {
        // ExpValAssign
        l += t.len;
        AsgExp *rhs = malloc(sizeof(AsgExp));
        l += parse_exp(src + l, err, rhs);
        if (err->tag != ERR_NONE) {
          free(rhs);
          return l;
        }
        data->tag = EXP_VAL_ASSIGN;
        data->str.len = l - leading_ws;
        memmove(&data->val_assign.lhs, &data->val, sizeof(AsgPattern));
        data->val_assign.rhs = rhs;
        return l;
      } else {
        // ExpVal
        data->tag = EXP_VAL;
        data->str.len = l - leading_ws;
        return l;
      }
    case IF:
      l += t.len;
      AsgExp *cond = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, cond);
      if (err->tag != ERR_NONE) {
        free(cond);
        return l;
      }

      l += parse_block(src + l, err, &data->exp_if.if_block);
      if (err->tag != ERR_NONE) {
        free(cond);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != ELSE) {
        data->tag = EXP_IF;
        data->str.len = l - leading_ws;
        data->exp_if.cond = cond;
        data->exp_if.else_block.str.start = NULL;
        data->exp_if.else_block.str.len = 0;
        data->exp_if.else_block.exps = NULL;
        data->exp_if.else_block.attrs = NULL;
        return l;
      } else {
        t = tokenize(src + l);
        if (t.tt == IF) {
          // treat this as a block containing a single expression
          data->exp_if.else_block.exps = NULL;
          data->exp_if.else_block.attrs = NULL;
          AsgExp *exp = sb_add(data->exp_if.else_block.exps, 1);
          l += parse_exp(src + l, err, exp);
          if (err->tag != ERR_NONE) {
            free(cond);
            free_inner_block(data->exp_if.if_block);
            free_inner_block(data->exp_if.else_block);
            return l;
          }

          data->tag = EXP_IF;
          data->str.len = l - leading_ws;
          data->exp_if.cond = cond;
          data->exp_if.else_block.str.start = exp->str.start;
          data->exp_if.else_block.str.len = exp->str.len;
          return l;
        } else {
          l += parse_block(src + l, err, &data->exp_if.else_block);
          if (err->tag != ERR_NONE) {
            free(cond);
            free_inner_block(data->exp_if.if_block);
            return l;
          }

          data->tag = EXP_IF;
          data->str.len = l - leading_ws;
          data->exp_if.cond = cond;
          return l;
        }
      }
    case WHILE:
      l += t.len;
      AsgExp *cond_while = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, cond_while);
      if (err->tag != ERR_NONE) {
        free(cond_while);
        return l;
      }

      l += parse_block(src + l, err, &data->exp_while.block);
      if (err->tag != ERR_NONE) {
        free(cond_while);
        return l;
      }

      data->tag = EXP_WHILE;
      data->str.len = l - leading_ws;
      data->exp_while.cond = cond_while;
      return l;
    case CASE:
      l += t.len;
      AsgExp *matcher_case = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, matcher_case);
      if (err->tag != ERR_NONE) {
        free(matcher_case);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != LBRACE) {
        err->tag = ERR_EXP;
        err->tt = t.tt;
        free(matcher_case);
        return l;
      }

      AsgPattern *patterns_case = NULL;
      AsgBlock *blocks_case = NULL;

      t = tokenize(src + l);
      while (t.tt != RBRACE) {
        AsgPattern *pattern = sb_add(patterns_case, 1);
        l += parse_pattern(src + l, err, pattern);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free(matcher_case);
          free_sb_patterns(patterns_case);
          free_sb_blocks(blocks_case);
          return l;
        }

        AsgBlock *block = sb_add(blocks_case, 1);
        l += parse_block(src + l, err, block);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free(matcher_case);
          free_sb_patterns(patterns_case);
          free_sb_blocks(blocks_case);
          return l;
        }

        t = tokenize(src + l);
      }
      l += t.len;

      data->tag = EXP_CASE;
      data->str.len = l - leading_ws;
      data->exp_case.matcher = matcher_case;
      data->exp_case.patterns = patterns_case;
      data->exp_case.blocks = blocks_case;
      return l;
    case LOOP:
      l += t.len;
      AsgExp *matcher_loop = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, matcher_loop);
      if (err->tag != ERR_NONE) {
        free(matcher_loop);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != LBRACE) {
        err->tag = ERR_EXP;
        err->tt = t.tt;
        free(matcher_loop);
        return l;
      }

      AsgPattern *patterns_loop = NULL;
      AsgBlock *blocks_loop = NULL;

      t = tokenize(src + l);
      while (t.tt != RBRACE) {
        AsgPattern *pattern = sb_add(patterns_loop, 1);
        l += parse_pattern(src + l, err, pattern);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free(matcher_loop);
          free_sb_patterns(patterns_loop);
          free_sb_blocks(blocks_loop);
          return l;
        }

        AsgBlock *block = sb_add(blocks_loop, 1);
        l += parse_block(src + l, err, block);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free(matcher_loop);
          free_sb_patterns(patterns_loop);
          free_sb_blocks(blocks_loop);
          return l;
        }

        t = tokenize(src + l);
      }
      l += t.len;

      data->tag = EXP_LOOP;
      data->str.len = l - leading_ws;
      data->exp_loop.matcher = matcher_loop;
      data->exp_loop.patterns = patterns_loop;
      data->exp_loop.blocks = blocks_loop;
      return l;
    case RETURN:
      l += t.len;
      AsgExp *inner_return = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_return);
      if (err->tag != ERR_NONE) {
        free(inner_return);
      }
      data->tag = EXP_RETURN;
      data->str.len = l - leading_ws;
      data->exp_return = inner_return;
      return l;
    case BREAK:
      l += t.len;
      AsgExp *inner_break = malloc(sizeof(AsgExp));
      l += parse_exp(src + l, err, inner_break);
      if (err->tag != ERR_NONE) {
        free(inner_break);
      }
      data->tag = EXP_BREAK;
      data->str.len = l - leading_ws;
      data->exp_break = inner_break;
      return l;
    case GOTO:
      l += t.len;
      l += parse_sid(src + l, err, &data->exp_goto);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
      }
      data->tag = EXP_GOTO;
      data->str.len = l - leading_ws;
      return l;
    case LABEL:
      l += t.len;
      l += parse_sid(src + l, err, &data->exp_label);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_EXP;
      }
      data->tag = EXP_LABEL;
      data->str.len = l - leading_ws;
      return l;
    default:
    err->tag = ERR_EXP;
    err->tt = t.tt;
    data->str.len = t.len;
    return t.len;
  }
}

size_t parse_exp(const char *src, ParserError *err, AsgExp *data) {
  size_t l = parse_exp_non_left_recursive(src, err, data);
  Token t = tokenize(src + l);

  while (true) {
    switch (t.tt) {
      case AT:
        l += t.len;
        AsgExp *l_deref = malloc(sizeof(AsgExp));
        memcpy(l_deref, data, sizeof(AsgExp));
        data->tag = EXP_DEREF;
        data->str.start = l_deref->str.start;
        data->str.len = l_deref->str.len + t.len;
        data->deref = l_deref;
        t = tokenize(src + l);
        break;
      case TILDE:
        l += t.len;
        AsgExp *l_deref_mut = malloc(sizeof(AsgExp));
        memcpy(l_deref_mut, data, sizeof(AsgExp));
        data->tag = EXP_DEREF_MUT;
        data->str.start = l_deref_mut->str.start;
        data->str.len = l_deref_mut->str.len + t.len;
        data->deref_mut = l_deref_mut;
        t = tokenize(src + l);
        break;
      case LBRACKET:
        l += t.len;

        AsgExp *array_index = malloc(sizeof(AsgExp));
        l += parse_exp(src + l, err, array_index);
        if (err->tag != ERR_NONE) {
          free(array_index);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != RBRACKET) {
          err->tag = ERR_EXP;
          err->tt = t.tt;
          free(array_index);
          free_inner_exp(*data);
          return l;
        }

        AsgExp *l_arr = malloc(sizeof(AsgExp));
        memcpy(l_arr, data, sizeof(AsgExp));
        data->tag = EXP_ARRAY_INDEX;
        data->str.start = l_arr->str.start;
        data->str.len = l - (l_arr->str.start - src);
        data->array_index.arr = l_arr;
        data->array_index.index = array_index;
        t = tokenize(src + l);
        break;
      case DOT:
        l += t.len;

        t = tokenize(src + l);
        switch (t.tt) {
          case INT:
            ;
            unsigned long field = strtoul(src + l, NULL, 10);
            l += t.len;

            AsgExp *l_product_access_anon = malloc(sizeof(AsgExp));
            memcpy(l_product_access_anon, data, sizeof(AsgExp));
            data->tag = EXP_PRODUCT_ACCESS_ANON;
            data->str.start = l_product_access_anon->str.start;
            data->str.len = l - (l_product_access_anon->str.start - src);
            data->product_access_anon.inner = l_product_access_anon;
            data->product_access_anon.field = field;
            t = tokenize(src + l);
            break;
          case ID:
            ;
            AsgExp *l_product_access_named = malloc(sizeof(AsgExp));
            memcpy(l_product_access_named, data, sizeof(AsgExp));
            l += parse_sid(src + l, err, &data->product_access_named.field);
            data->tag = EXP_PRODUCT_ACCESS_NAMED;
            data->str.start = l_product_access_named->str.start;
            data->str.len = l - (l_product_access_named->str.start - src);
            data->product_access_named.inner = l_product_access_named;
            t = tokenize(src + l);
            break;
          default:
            l += t.len;
            free_inner_exp(*data);
            err->tag = ERR_EXP;
            err->tt = t.tt;
            return l;
        }
        break;
      case LPAREN:
        // handles empty fun application, anon fun application, named fun application
        l += t.len;
        t = tokenize(src + l);

        AsgExp *l_fun_app = malloc(sizeof(AsgExp));
        memcpy(l_fun_app, data, sizeof(AsgExp));

        if (t.tt == RPAREN) {
          // empty (anon) fun application
          l += t.len;

          data->tag = EXP_FUN_APP_ANON;
          data->str.start = l_fun_app->str.start;
          data->str.len = l - (l_fun_app->str.start - src);
          data->fun_app_anon.fun = l_fun_app;
          data->fun_app_anon.args = NULL;
          t = tokenize(src + l);
          break;
        } else if (t.tt == ID) {
          // named fun app iff the next token is EQ
          Token t2 = tokenize(src + l + t.len);
          if (t2.tt == EQ) {
            // named fun app
            AsgSid *sids = NULL;
            AsgExp *inners = NULL;

            AsgSid *sid = sb_add(sids, 1);
            l += parse_sid(src + l, err, sid);
            l += t2.len;
            AsgExp *inner;
            inner = sb_add(inners, 1);
            l += parse_exp(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              sb_free(sids);
              free_sb_exps(inners);
              free(l_fun_app);
              return l;
            }
            t = tokenize(src + l);
            l += t.len;

            while (t.tt == COMMA) {
              sid = sb_add(sids, 1);
              l += parse_sid(src + l, err, sid);
              if (err->tag != ERR_NONE) {
                sb_free(sids);
                free_sb_exps(inners);
                free(l_fun_app);
                err->tag = ERR_EXP;
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
              if (t.tt != EQ) {
                sb_free(sids);
                free_sb_exps(inners);
                free(l_fun_app);
                err->tag = ERR_EXP;
                err->tt = t.tt;
                return l;
              }

              inner = sb_add(inners, 1);
              l += parse_exp(src + l, err, inner);
              if (err->tag != ERR_NONE) {
                sb_free(sids);
                free_sb_exps(inners);
                free(l_fun_app);
                return l;
              }

              t = tokenize(src + l);
              l += t.len;
            }

            data->tag = EXP_FUN_APP_NAMED;
            data->str.start = l_fun_app->str.start;
            data->str.len = l - (l_fun_app->str.start - src);
            data->fun_app_named.fun = l_fun_app;
            data->fun_app_named.args = inners;
            data->fun_app_named.sids = sids;
            t = tokenize(src + l);
            break;
          }
        }
        // anon fun app
        AsgExp *inners = NULL;
        AsgExp *inner = sb_add(inners, 1);
        l += parse_exp(src + l, err, inner);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          free_sb_exps(inners);
          free(l_fun_app);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != COMMA && t.tt != RPAREN) {
          err->tag = ERR_EXP;
          free_sb_exps(inners);
          free(l_fun_app);
          err->tt = t.tt;
          return l;
        } else {
          while (t.tt == COMMA) {
            inner = sb_add(inners, 1);
            l += parse_exp(src + l, err, inner);
            if (err->tag != ERR_NONE) {
              free_sb_exps(inners);
              free(l_fun_app);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }
          if (t.tt != RPAREN) {
            err->tag = ERR_EXP;
            free_sb_exps(inners);
            free(l_fun_app);
            err->tt = t.tt;
            return l;
          }

          data->tag = EXP_FUN_APP_ANON;
          data->str.start = l_fun_app->str.start;
          data->str.len = l - (l_fun_app->str.start - src);
          data->fun_app_anon.fun = l_fun_app;
          data->fun_app_anon.args = inners;
          t = tokenize(src + l);
          break;
        }
      case AS:
        l += t.len;

        AsgType *cast_type = malloc(sizeof(AsgType));
        l += parse_type(src + l, err, cast_type);
        if (err->tag != ERR_NONE) {
          free(cast_type);
          err->tag = ERR_EXP;
          return l;
        }

        AsgExp *l_cast = malloc(sizeof(AsgExp));
        memcpy(l_cast, data, sizeof(AsgExp));
        data->tag = EXP_CAST;
        data->str.start = l_cast->str.start;
        data->str.len = l - (l_cast->str.start - src);
        data->cast.inner = l_cast;
        data->cast.type = cast_type;
        t = tokenize(src + l);
        break;
      case PLUS:
      case MINUS:
      case TIMES:
      case DIV:
      case MOD:
      case PIPE:
      case AMPERSAND:
      case XOR:
      case LAND:
      case LOR:
      case EQUALS:
      case NOTEQUALS:
        ;
        AsgBinOp bin_op;
        l += parse_bin_op(src + l, err, &bin_op);
        if (err->tag != ERR_NONE) {
          if (err->tag != ERR_NONE) {
            err->tag = ERR_EXP;
            return l;
          }
        }

        AsgExp *bin_op_rhs = malloc(sizeof(AsgExp));
        l += parse_exp(src + l, err, bin_op_rhs);
        if (err->tag != ERR_NONE) {
          free(bin_op_rhs);
          return l;
        }

        AsgExp *l_bin_op = malloc(sizeof(AsgExp));
        memcpy(l_bin_op, data, sizeof(AsgExp));
        data->tag = EXP_BIN_OP;
        data->str.start = l_bin_op->str.start;
        data->str.len = l - (l_bin_op->str.start - src);
        data->bin_op.op = bin_op;
        data->bin_op.lhs = l_bin_op;
        data->bin_op.rhs = bin_op_rhs;
        t = tokenize(src + l);
        break;
      case PLUS_ASSIGN:
      case MINUS_ASSIGN:
      case TIMES_ASSIGN:
      case DIV_ASSIGN:
      case MOD_ASSIGN:
      case XOR_ASSIGN:
      case AND_ASSIGN:
      case OR_ASSIGN:
      case EQ:
        ;
        AsgAssignOp op;
        l += parse_assign_op(src + l, err, &op);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_EXP;
          return l;
        }

        AsgExp *assign_rhs = malloc(sizeof(AsgExp));
        l += parse_exp(src + l, err, assign_rhs);
        if (err->tag != ERR_NONE) {
          free(assign_rhs);
          return l;
        }

        AsgExp *l_assign = malloc(sizeof(AsgExp));
        memcpy(l_assign, data, sizeof(AsgExp));
        data->tag = EXP_ASSIGN;
        data->str.start = l_assign->str.start;
        data->str.len = l - (l_assign->str.start - src);
        data->assign.op = op;
        data->assign.lhs = l_assign;
        data->assign.rhs = assign_rhs;
        t = tokenize(src + l);
        break;
      case LANGLE:
      case RANGLE:
        ;
        AsgAssignOp foo_assign_op;
        size_t assign_len = parse_assign_op(src + l, err, &foo_assign_op);
        if (err->tag != ERR_NONE) {
          AsgBinOp bin_op;
          l += parse_bin_op(src + l, err, &bin_op);
          if (err->tag != ERR_NONE) {
            if (err->tag != ERR_NONE) {
              err->tag = ERR_EXP;
              return l;
            }
          }

          AsgExp *bin_op_rhs = malloc(sizeof(AsgExp));
          l += parse_exp(src + l, err, bin_op_rhs);
          if (err->tag != ERR_NONE) {
            free(bin_op_rhs);
            return l;
          }

          AsgExp *l_bin_op = malloc(sizeof(AsgExp));
          memcpy(l_bin_op, data, sizeof(AsgExp));
          data->tag = EXP_BIN_OP;
          data->str.start = l_bin_op->str.start;
          data->str.len = l - (l_bin_op->str.start - src);
          data->bin_op.op = bin_op;
          data->bin_op.lhs = l_bin_op;
          data->bin_op.rhs = bin_op_rhs;
          t = tokenize(src + l);
          break;
        }
        l += assign_len;

        AsgExp *foo_assign_rhs = malloc(sizeof(AsgExp));
        l += parse_exp(src + l, err, foo_assign_rhs);
        if (err->tag != ERR_NONE) {
          free(foo_assign_rhs);
          return l;
        }

        AsgExp *foo_l_assign = malloc(sizeof(AsgExp));
        memcpy(foo_l_assign, data, sizeof(AsgExp));
        data->tag = EXP_ASSIGN;
        data->str.start = foo_l_assign->str.start;
        data->str.len = l - (foo_l_assign->str.start - src);
        data->assign.op = foo_assign_op;
        data->assign.lhs = foo_l_assign;
        data->assign.rhs = foo_assign_rhs;
        t = tokenize(src + l);
        break;
      default:
        return l;
    }
  }
}

void free_inner_exp(AsgExp data) {
  switch (data.tag) {
    case EXP_ID:
      free_inner_id(data.id);
      break;
    case EXP_REF:
      free_inner_exp(*data.ref);
      free(data.ref);
      break;
    case EXP_REF_MUT:
      free_inner_exp(*data.ref_mut);
      free(data.ref_mut);
      break;
    case EXP_ARRAY:
      free_inner_exp(*data.array);
      free(data.array);
      break;
    case EXP_PRODUCT_REPEATED:
      free_inner_exp(*data.product_repeated.inner);
      free(data.product_repeated.inner);
      free_inner_repeat(data.product_repeated.repeat);
    break;
    case EXP_PRODUCT_ANON:
      free_sb_exps(data.product_anon);
      break;
    case EXP_PRODUCT_NAMED:
      free_sb_exps(data.product_named.inners);
      sb_free(data.product_named.sids);
      break;
    case EXP_SIZE_OF:
      free_inner_type(*data.size_of);
      free(data.size_of);
      break;
    case EXP_ALIGN_OF:
      free_inner_type(*data.align_of);
      free(data.align_of);
      break;
    case EXP_NOT:
      free_inner_exp(*data.exp_not);
      free(data.exp_not);
      break;
    case EXP_NEGATE:
      free_inner_exp(*data.exp_negate);
      free(data.exp_negate);
      break;
    case EXP_VAL:
      free_inner_pattern(data.val);
      break;
    case EXP_VAL_ASSIGN:
      free_inner_pattern(data.val_assign.lhs);
      free_inner_exp(*data.val_assign.rhs);
      free(data.val_assign.rhs);
      break;
    case EXP_BLOCK:
      free_inner_block(data.block);
      break;
    case EXP_IF:
      free_inner_exp(*data.exp_if.cond);
      free(data.exp_if.cond);
      free_inner_block(data.exp_if.if_block);
      free_inner_block(data.exp_if.else_block);
      break;
    case EXP_WHILE:
      free_inner_exp(*data.exp_while.cond);
      free(data.exp_while.cond);
      free_inner_block(data.exp_while.block);
      break;
    case EXP_CASE:
      free_inner_exp(*data.exp_case.matcher);
      free(data.exp_case.matcher);
      free_sb_patterns(data.exp_case.patterns);
      free_sb_blocks(data.exp_case.blocks);
      break;
    case EXP_LOOP:
      free_inner_exp(*data.exp_loop.matcher);
      free(data.exp_loop.matcher);
      free_sb_patterns(data.exp_loop.patterns);
      free_sb_blocks(data.exp_loop.blocks);
      break;
    case EXP_RETURN:
      free_inner_exp(*data.exp_return);
      free(data.exp_return);
      break;
    case EXP_BREAK:
      free_inner_exp(*data.exp_break);
      free(data.exp_break);
      break;
    case EXP_DEREF:
      free_inner_exp(*data.deref);
      free(data.deref);
      break;
    case EXP_DEREF_MUT:
      free_inner_exp(*data.deref_mut);
      free(data.deref_mut);
      break;
    case EXP_ARRAY_INDEX:
      free_inner_exp(*data.array_index.arr);
      free(data.array_index.arr);
      free_inner_exp(*data.array_index.index);
      free(data.array_index.index);
      break;
    case EXP_PRODUCT_ACCESS_ANON:
      free_inner_exp(*data.product_access_anon.inner);
      free(data.product_access_anon.inner);
      break;
    case EXP_PRODUCT_ACCESS_NAMED:
      free_inner_exp(*data.product_access_named.inner);
      free(data.product_access_named.inner);
      break;
    case EXP_FUN_APP_ANON:
      free_inner_exp(*data.fun_app_anon.fun);
      free(data.fun_app_anon.fun);
      free_sb_exps(data.fun_app_anon.args);
      break;
    case EXP_FUN_APP_NAMED:
      free_inner_exp(*data.fun_app_named.fun);
      free(data.fun_app_named.fun);
      free_sb_exps(data.fun_app_named.args);
      sb_free(data.fun_app_named.sids);
      break;
    case EXP_CAST:
      free_inner_exp(*data.cast.inner);
      free(data.cast.inner);
      free_inner_type(*data.cast.type);
      free(data.cast.type);
      break;
    case EXP_BIN_OP:
      free_inner_exp(*data.bin_op.lhs);
      free(data.bin_op.lhs);
      free_inner_exp(*data.bin_op.rhs);
      free(data.bin_op.rhs);
      break;
    case EXP_ASSIGN:
      free_inner_exp(*data.assign.lhs);
      free(data.assign.lhs);
      free_inner_exp(*data.assign.rhs);
      free(data.assign.rhs);
      break;
    default:
      return;
  }
}

void free_sb_use_tree(AsgUseTree *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_use_tree(sb[i]);
  }
  sb_free(sb);
}

size_t parse_sid_or_use_kw(const char *src, ParserError *err, AsgSid *data) {
  Token t = tokenize(src);

  if (t.tt != ID && t.tt != DEP && t.tt != MAGIC && t.tt != KW_MOD) {
    err->tag = ERR_SID;
    err->tt = t.tt;
    return t.len;
  } else {
    err->tag = ERR_NONE;
  }

  data->str.start = src + (t.len - t.token_len);
  data->str.len = t.token_len;

  return t.len;
}

size_t parse_use_tree(const char *src, ParserError *err, AsgUseTree *data) {
  Token t;
  err->tag = ERR_NONE;
  data->str.start = src;
  size_t l = 0;

  l += parse_sid_or_use_kw(src, err, &data->sid);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_USE_TREE;
    return l;
  }

  t = tokenize(src + l);
  if (t.tt == AS) {
    l += t.len;
    l += parse_sid(src + l, err, &data->rename);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_USE_TREE;
      return l;
    }

    data->str.len = l;
    data->tag = USE_TREE_RENAME;
    return l;
  } else if (t.tt == SCOPE) {
    l += t.len;
    t = tokenize(src + l);
    if (t.tt == ID || t.tt == DEP || t.tt == MAGIC || t.tt == KW_MOD) {
      AsgUseTree *inners = NULL;
      AsgUseTree *inner = sb_add(inners, 1);
      l += parse_use_tree(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        free_sb_use_tree(inners);
        return l;
      }
      data->str.len = l;
      data->tag = USE_TREE_BRANCH;
      data->branch = inners;
      return l;
    } else if (t.tt == LBRACE) {
      l += t.len;

      AsgUseTree *inners = NULL;
      AsgUseTree *inner = sb_add(inners, 1);
      l += parse_use_tree(src + l, err, inner);
      if (err->tag != ERR_NONE) {
        free_sb_use_tree(inners);
        return l;
      }

      t = tokenize(src + l);
      l += t.len;
      if (t.tt != COMMA && t.tt != RBRACE) {
        err->tag = ERR_USE_TREE;
        free_sb_use_tree(inners);
        err->tt = t.tt;
        return l;
      } else {
        while (t.tt == COMMA) {
          inner = sb_add(inners, 1);
          l += parse_use_tree(src + l, err, inner);
          if (err->tag != ERR_NONE) {
            free_sb_use_tree(inners);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }
        if (t.tt != RBRACE) {
          err->tag = ERR_EXP;
          free_sb_use_tree(inners);
          err->tt = t.tt;
          return l;
        }

        data->str.len = l;
        data->tag = USE_TREE_BRANCH;
        data->branch = inners;
        return l;
      }
    } else {
      err->tag = ERR_USE_TREE;
      err->tt = t.tt;
      return l;
    }
  } else {
    data->str.len = l;
    data->tag = USE_TREE_LEAF;
    return l;
  }
}

void free_inner_use_tree(AsgUseTree data) {
  switch (data.tag) {
    case USE_TREE_BRANCH:
      free_sb_use_tree(data.branch);
      break;
    default:
      return;
  }
}

void free_sb_items(AsgItem *sb) {
  int i;
  int count = sb_count(sb);
  for (i = 0; i < count; i++) {
    free_inner_item(sb[i]);
  }
  sb_free(sb);
}

size_t parse_item(const char *src, ParserError *err, AsgItem *data) {
  Token t;
  err->tag = ERR_NONE;
  data->str.start = src;
  size_t l = 0;

  t = tokenize(src);
  if (t.tt == PUB) {
    data->pub = true;
    l += t.len;
    t = tokenize(src + l);
  } else {
    data->pub = false;
  }
  l += t.len;

  switch (t.tt) {
    case USE:
      l += parse_use_tree(src + l, err, &data->use);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      data->tag = ITEM_USE;
      data->str.len = l;
      return l;
    case TYPE:
      l += parse_sid(src + l, err, &data->type.sid);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      t = tokenize(src + l);
      if (t.tt != EQ) {
        err->tag = ERR_ITEM;
        err->tt = t.tt;
        return l;
      }
      l += t.len;

      l += parse_type(src + l, err, &data->type.type);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      data->tag = ITEM_TYPE;
      data->str.len = l;
      return l;
    case VAL:
      t = tokenize(src + l);
      if (t.tt == MUT) {
        data->val.mut = true;
        l += t.len;
        t = tokenize(src + l);
      } else {
        data->val.mut = false;
      }

      l += parse_sid(src + l, err, &data->val.sid);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      t = tokenize(src + l);
      if (t.tt != EQ) {
        err->tag = ERR_ITEM;
        err->tt = t.tt;
        return l;
      }
      l += t.len;

      l += parse_exp(src + l, err, &data->val.exp);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      data->tag = ITEM_VAL;
      data->str.len = l;
      return l;
    case FN:
      l += parse_sid(src + l, err, &data->type.sid);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        return l;
      }

      t = tokenize(src + l);
      if (t.tt != EQ) {
        err->tag = ERR_ITEM;
        err->tt = t.tt;
        return l;
      }
      l += t.len;

      t = tokenize(src + l);
      if (t.tt == LANGLE) {
        l += t.len;

        AsgSid *type_args = NULL;
        AsgSid *type_arg = sb_add(type_args, 1);
        l += parse_sid(src + l, err, type_arg);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_ITEM;
          sb_free(type_args);
          return l;
        }

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != COMMA && t.tt != RANGLE) {
          err->tag = ERR_ITEM;
          err->tt = t.tt;
          sb_free(type_args);
          return l;
        } else {
          while (t.tt == COMMA) {
            type_arg = sb_add(type_args, 1);
            l += parse_sid(src + l, err, type_arg);
            if (err->tag != ERR_NONE) {
              err->tag = ERR_ITEM;
              sb_free(type_args);
              return l;
            }

            t = tokenize(src + l);
            l += t.len;
          }
          if (t.tt != RANGLE) {
            err->tag = ERR_ITEM;
            err->tt = t.tt;
            sb_free(type_args);
            return l;
          }

          data->fun.type_args = type_args;

          t = tokenize(src + l);
          l += t.len;
          if (t.tt != FAT_ARROW) {
            err->tag = ERR_ITEM;
            err->tt = t.tt;
            sb_free(type_args);
            return l;
          }

          t = tokenize(src + l);
        }
      } else {
        data->fun.type_args = NULL;
      }

      if (t.tt != LPAREN) {
        err->tag = ERR_ITEM;
        err->tt = t.tt;
        sb_free(data->fun.type_args);
        return l;
      }
      l += t.len;

      t = tokenize(src + l);
      if (t.tt == RPAREN) {
        data->fun.arg_sids = NULL;
        data->fun.arg_types = NULL;
        l += t.len;
      } else {
        AsgSid *sids = NULL;
        AsgType *types = NULL;

        AsgSid *sid = sb_add(sids, 1);
        l += parse_sid(src + l, err, sid);

        t = tokenize(src + l);
        l += t.len;
        if (t.tt != COLON) {
          err->tag = ERR_ITEM;
          err->tt = t.tt;
          sb_free(data->fun.type_args);
          sb_free(sids);
          free_sb_types(types);
          return l;
        }

        AsgType *type;
        type = sb_add(types, 1);
        l += parse_type(src + l, err, type);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_ITEM;
          sb_free(data->fun.type_args);
          sb_free(sids);
          free_sb_types(types);
          return l;
        }
        t = tokenize(src + l);
        l += t.len;

        while (t.tt == COMMA) {
          sid = sb_add(sids, 1);
          l += parse_sid(src + l, err, sid);
          if (err->tag != ERR_NONE) {
            err->tag = ERR_ITEM;
            sb_free(data->fun.type_args);
            sb_free(sids);
            free_sb_types(types);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
          if (t.tt != COLON) {
            err->tag = ERR_ITEM;
            err->tt = t.tt;
            sb_free(data->fun.type_args);
            sb_free(sids);
            free_sb_types(types);
            return l;
          }

          type = sb_add(types, 1);
          l += parse_type(src + l, err, type);
          if (err->tag != ERR_NONE) {
            err->tag = ERR_ITEM;
            sb_free(data->fun.type_args);
            sb_free(sids);
            free_sb_types(types);
            return l;
          }

          t = tokenize(src + l);
          l += t.len;
        }

        if (t.tt != RPAREN) {
          err->tag = ERR_ITEM;
          err->tt = t.tt;
          sb_free(data->fun.type_args);
          sb_free(sids);
          free_sb_types(types);
          return l;
        }
        l += t.len;
        data->fun.arg_sids = sids;
        data->fun.arg_types = types;
      }

      t = tokenize(src + l);
      if (t.tt == ARROW) {
        l += t.len;
        l += parse_type(src + l, err, &data->fun.ret);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_ITEM;
          sb_free(data->fun.type_args);
          sb_free(data->fun.arg_sids);
          free_sb_types(data->fun.arg_types);
          return l;
        }
      } else {
        data->fun.ret.str.start = src + l;
        data->fun.ret.str.len = 0;
        data->fun.ret.tag = TYPE_PRODUCT_ANON;
        data->fun.ret.product_anon = NULL;
      }

      l += parse_block(src + l, err, &data->fun.body);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_ITEM;
        sb_free(data->fun.type_args);
        sb_free(data->fun.arg_sids);
        free_sb_types(data->fun.arg_types);
        free_inner_type(data->fun.ret);
        return l;
      }

      data->tag = ITEM_FUN;
      data->str.len = l;
      return l;
    case FFI:
      t = tokenize(src + l);
      if (t.tt == USE) {
        l += t.len;

        t = tokenize(src + l);
        if (t.tt != LPAREN) {
          err->tag = ERR_ITEM;
          err->tt = t.tt;
          return l;
        }
        l += t.len;
        data->ffi_include.include.start = src + l;
        data->ffi_include.include.len = 0;

        t = tokenize(src + l);
        while (t.tt != RPAREN) {
          l += t.len;
          data->ffi_include.include.len += t.len;
          t = tokenize(src + l); // This does not correctly handle EOF and lexer errors

          if (t.tt == END || token_type_error(t.tt)) {
            err->tag = ERR_ITEM;
            err->tt = t.tt;
            return l;
          }
        }
        l += t.len;

        data->tag = ITEM_FFI_INCLUDE;
        data->str.len = l;
        return l;
      } else {
        if (t.tt == MUT) {
          data->ffi_val.mut = true;
          l += t.len;
          t = tokenize(src + l);
        } else {
          data->ffi_val.mut = false;
        }

        l += parse_sid(src + l, err, &data->ffi_val.sid);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_ITEM;
          return l;
        }

        t = tokenize(src + l);
        if (t.tt != COLON) {
          err->tag = ERR_ITEM;
          err->tt = t.tt;
          return l;
        }
        l += t.len;


        l += parse_type(src + l, err, &data->ffi_val.type);
        if (err->tag != ERR_NONE) {
          err->tag = ERR_ITEM;
          return l;
        }

        data->tag = ITEM_FFI_VAL;
        data->str.len = l;
        return l;
      }
    default:
      err->tag = ERR_ITEM;
      err->tt = t.tt;
      data->str.len = l;
      return l;
  }
}

void free_inner_item(AsgItem data) {
  switch (data.tag) {
    case ITEM_USE:
      free_inner_use_tree(data.use);
      break;
    case ITEM_TYPE:
      free_inner_type(data.type.type);
      break;
    case ITEM_VAL:
      free_inner_exp(data.val.exp);
      break;
    case ITEM_FUN:
      sb_free(data.fun.type_args);
      sb_free(data.fun.arg_sids);
      free_sb_types(data.fun.arg_types);
      free_inner_type(data.fun.ret);
      free_inner_block(data.fun.body);
      break;
    case ITEM_FFI_VAL:
      free_inner_type(data.ffi_val.type);
      break;
    case ITEM_FFI_INCLUDE:
      break;
  }
}

size_t parse_file(const char *src, ParserError *err, AsgFile *data) {
  Token t;
  err->tag = ERR_NONE;
  data->str.start = src;
  size_t l = 0;

  AsgItem *items = NULL;
  AsgMeta **all_attrs = NULL; // sb of sbs

  t = tokenize(src + l);

  AsgMeta **attrs = sb_add(all_attrs, 1); // ptr to an sb
  *attrs = NULL;

  l += parse_attrs(src + l, err, attrs);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_FILE;
    free_sb_items(items);
    free_sb_sb_meta(all_attrs);
    return l;
  }

  AsgItem *item = sb_add(items, 1);
  l += parse_item(src + l, err, item);
  if (err->tag != ERR_NONE) {
    err->tag = ERR_FILE;
    free_sb_items(items);
    free_sb_sb_meta(all_attrs);
    return l;
  }

  t = tokenize(src + l);

  while (t.tt != END) {
    AsgMeta **attrs = sb_add(all_attrs, 1); // ptr to an sb
    *attrs = NULL;
    l += parse_attrs(src + l, err, attrs);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_FILE;
      free_sb_items(items);
      free_sb_sb_meta(all_attrs);
      return l;
    }

    AsgItem *item = sb_add(items, 1);
    l += parse_item(src + l, err, item);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_FILE;
      free_sb_items(items);
      free_sb_sb_meta(all_attrs);
      return l;
    }

    t = tokenize(src + l);
  }

  data->str.len = l;
  data->items = items;
  data->attrs = all_attrs;
  data->bindings_by_sid = NULL;
  data->pub_bindings_by_sid = NULL;
  data->did_init_bindings_by_sid = false;
  return l;
}

void free_inner_file(AsgFile data) {
  free_sb_items(data.items);
  free_sb_sb_meta(data.attrs);

  if (data.did_init_bindings_by_sid) {
    raxFree(data.bindings_by_sid);
    raxFree(data.pub_bindings_by_sid);
    sb_free(data.bindings);
  }
}
