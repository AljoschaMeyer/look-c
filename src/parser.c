#include <stdlib.h>
#include <stdio.h> // TODO remove this after printf debugging

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
