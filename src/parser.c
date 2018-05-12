#include <stdlib.h>
#include <stdio.h> // TODO remove this after printf debugging
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "asg.h"
#include "stretchy_buffer.h"

// Consumes a token, but with the generic parsing function signature.
// TODO delete if this remains unused
size_t parse_token(const char *src, OoError *err, void *data) {
  (void)err;
  *((Token *) data) = tokenize(src);
  return ((Token *) data)->len;
}

size_t parse_id(const char *src, OoError *err, AsgId *data) {
  size_t l = 0;
  Token t;

  err->tag = ERR_NONE;
  data->sids = NULL;
  sb_add(data->sids, 1);

  l += parse_sid(src, err, &sb_last(data->sids));
  if (err->tag != ERR_NONE) {
    sb_free(data->sids);
    return l;
  }

  t = tokenize(src + l);
  l += t.len;
  while (t.tt == SCOPE) {
    sb_add(data->sids, 1);
    l += parse_sid(src + l, err, &sb_last(data->sids));
    if (err->tag != ERR_NONE) {
      sb_free(data->sids);
      return l;
    }

    t = tokenize(src + l);
    l += t.len;
  }
  l -= t.len;

  data->len = l;
  return l;
}

void free_inner_id(AsgId data) {
  sb_free(data.sids);
}

size_t parse_sid(const char *src, OoError *err, AsgSid *data) {
  Token t = tokenize(src);

  if (t.tt != ID) {
    err->tag = ERR_SID;
    err->tt = t.tt;
    return t.len;
  } else {
    err->tag = ERR_NONE;
  }

  data->src = src + (t.len - t.token_len);
  data->len = t.token_len;

  return t.len;
}

size_t parse_macro_inv(const char *src, OoError *err, AsgMacroInv *data) {
  err->tag = ERR_MACRO_INV;

  size_t l = 0;
  Token t;

  t = tokenize(src);
  l += t.len;
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
  data->name = tmp.src;
  data->name_len = tmp.len;

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

  data->args = src + args_start;
  data->args_len = l - (args_start + t.len); // last RPAREN is not part of the args

  err->tag = ERR_NONE;
  data->src = src;
  data->len = l;
  return l;
}

size_t parse_literal(const char *src, OoError *err, AsgLiteral *data) {
  Token t = tokenize(src);
  err->tag = ERR_NONE;
  data->src = src + (t.len - t.token_len);
  data->len = t.token_len;

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

size_t parse_bin_op(const char *src, OoError *err, AsgBinOp *op) {
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

size_t parse_size_of(const char *src, OoError *err, AsgType *data) {
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

size_t parse_align_of(const char *src, OoError *err, AsgType *data) {
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

size_t parse_repeat(const char *src, OoError *err, AsgRepeat *data) {
  Token t = tokenize(src);
  err->tag = ERR_NONE;
  data->src = src + (t.len - t.token_len);
  size_t l = 0;

  if (t.tt == INT) {
    l += t.len;
    data->len = t.token_len;
    data->tag = REPEAT_INT;
  } else if (t.tt == DOLLAR) {
    l += parse_macro_inv(src, err, &(data->macro));
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->len = l;
    data->tag = REPEAT_MACRO;
  } else if (t.tt == SIZEOF) {
    l += parse_size_of(src, err, data->size_of);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->len = l;
    data->tag = REPEAT_SIZE_OF;
  } else if (t.tt == ALIGNOF) {
    l += parse_align_of(src, err, data->align_of);
    if (err->tag != ERR_NONE) {
      err->tag = ERR_REPEAT;
      return l;
    }
    data->len = l;
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

  data->len = l;
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

size_t parse_type(const char *src, OoError *err, AsgType *data) {
  Token t = tokenize(src);
  err->tag = ERR_NONE;
  data->src = src + (t.len - t.token_len);
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
              data->len = l;
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
          data->len = l;
          data->app_anon.tlf = id;
          data->app_anon.args = inners;
          return l;
        }
      } else {
        // not a type application, just an id
        data->tag = TYPE_ID;
        data->id = id;
        data->len = data->id.len;
        return l;
      }
    case DOLLAR:
      l += parse_macro_inv(src, err, &data->macro);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
      }
      data->tag = TYPE_MACRO;
      data->len = data->macro.len;
      return l;
    case AT:
      l += t.len;
      AsgType *inner_ptr = malloc(sizeof(AsgType));
      l += parse_type(src + l, err, inner_ptr);
      if (err->tag != ERR_NONE) {
        free(inner_ptr);
      }
      data->tag = TYPE_PTR;
      data->len = l;
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
      data->len = l;
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
      data->len = l;
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
        data->len = l;
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
          data->len = l;
          data->fun_anon.args = NULL;
          data->fun_anon.ret = ret;
          return l;
        } else {
          data->len = l;
          data->tag = TYPE_PRODUCT_ANON;
          data->product_anon = NULL;
          return l;
        }
      } else if (t.tt == ID) {
        // named fun, named product iff the next token is EQUALS
        Token t2 = tokenize(src + l + t.len);
        if (t2.tt == EQ) {
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
            data->len = l;
            data->fun_named.arg_types = types;
            data->fun_named.arg_sids = sids;
            data->fun_named.ret = ret;
            return l;
          }
          data->tag = TYPE_PRODUCT_NAMED;
          data->len = l;
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
        data->len = l;
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
          data->len = l;
          data->fun_anon.args = inners;
          data->fun_anon.ret = ret;
          return l;
        } else {
          data->tag = TYPE_PRODUCT_ANON;
          data->len = l;
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
      data->len = l;
      data->sum.pub = pub;
      data->sum.summands = summands;
      return l;
    default:
      err->tag = ERR_TYPE;
      err->tt = t.tt;
      data->len = t.len;
      return t.len;
  }
}

size_t parse_summand(const char *src, OoError *err, AsgSummand *data) {
  data->src = src;
  size_t l = 0;
  Token t = tokenize(src);
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
          data->len = l;
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
      data->len = l;
      data->anon = inners;
      return l;
    }
  } else {
    // Identifier without parens (empty anon)
    data->len = l;
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

size_t parse_pattern(const char *src, OoError *err, AsgPattern *data) {
  Token t = tokenize(src);
  err->tag = ERR_NONE;
  data->src = src + (t.len - t.token_len);
  size_t l = 0;

  bool mut = false;
  AsgType *type = NULL;
  AsgId id;
  switch (t.tt) {
    case UNDERSCORE:
      l += t.len;
      data->tag = PATTERN_BLANK;
      data->len = l;
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
      data->len = l;
      data->id.mut = mut;
      data->id.type = type;
      return l;
    case INT:
    case FLOAT:
    case STRING:
      l += parse_literal(src + l, err, &data->lit);
      data->len = l;
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
      data->len = l;
      data->ptr = inner_ptr;
      return l;
    case LPAREN:
      l += t.len;

      t = tokenize(src + l);
      if (t.tt == RPAREN) {
        l += t.len;
        data->tag = PATTERN_PRODUCT_ANON;
        data->len = l;
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
            data->len = l;
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
        data->len = l;
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
              data->len = l;
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
          data->len = l;
          data->summand_anon.id = id;
          data->summand_anon.fields = inners;
          return l;
        }
      } else {
        // Identifier without parens (empty anon)
        data->len = l;
        data->tag = PATTERN_SUMMAND_ANON;
        data->summand_anon.id = id;
        data->summand_anon.fields = NULL;
        return l;
      }
    default:
      err->tag = ERR_TYPE;
      err->tt = t.tt;
      data->len = t.len;
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
