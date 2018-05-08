#include <stdlib.h>
#include <stdio.h> // TODO remove this after printf debugging
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "asg.h"

// Consumes a token, but with the generic parsing function signature.
size_t parse_token(const char *src, OoError *err, void *data) {
  (void)err;
  *((Token *) data) = tokenize(src);
  return ((Token *) data)->len; 
}

size_t sep_1(const char *src, OoError *err,
  size_t parse (const char *, OoError *, void *),
  size_t data_size, TokenType tt,
  void **data_array, size_t *data_len) {
  // This works by parsing the src twice: Once to determine how many
  // data items there are, and then again, placing the data in an
  // array of the correct length.
  // TODO use a growable array instead. The twice-parsing approach leads
  // to quadratic running time in the depth of the nested parsing.
  void *ignored_data = malloc(data_size); // data slot for the first parsing attempt
  size_t len;
  Token sep;

  len = parse(src, err, ignored_data);
  if (err->tag != ERR_NONE) {
    free(ignored_data);
    return len;
  }
  *data_len = 1;

  sep = tokenize(src + len);
  len += sep.len;
  while (sep.tt == tt) {
    len += parse(src + len, err, ignored_data);
    if (err-> tag != ERR_NONE) {
      free(ignored_data);
      return len;
    }
    *data_len += 1;

    sep = tokenize(src + len);
    len += sep.len;
  }
  len -= sep.len; // subtract length of non-separator token

  free(ignored_data);
  *data_array = malloc(data_size * (*data_len));
  len = parse(src, err, *data_array);

  size_t i;
  for (i = 1; i < *data_len; i++) {
    sep = tokenize(src + len);
    len += sep.len;
    len += parse(src + len, err, (((char *) *data_array) + (data_size * i)));
  }

  err->tag = ERR_NONE;
  return len;
}

size_t parse_sid_void(const char *src, OoError *err, void *data) {
  return parse_sid(src, err, (AsgSid *) data);
}

size_t parse_id(const char *src, OoError *err, AsgId *data) {
  return sep_1(
    src, err,
    parse_sid_void, sizeof(AsgSid), SCOPE,
    (void **) &(data->sids), &(data->sids_len)
  );
}

void free_inner_id(AsgId data) {
  free(data.sids);
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

size_t parse_type(const char *src, OoError *err, AsgType *data) {
  Token t = tokenize(src);
  err->tag = ERR_NONE;
  data->src = src + (t.len - t.token_len); 
  size_t l = 0;

  switch (t.tt) {
    case ID:
      l += parse_id(src, err, &data->id);
      if (err->tag != ERR_NONE) {
        err->tag = ERR_TYPE;
      }
      return l;
    default:
      // TODO no default
      *((char *) NULL) = 42;
      return 42;
  }
}


