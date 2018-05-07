#include <stdlib.h>
#include <stdio.h> // TODO remove this after printf debugging

#include "lexer.h"
#include "parser.h"
#include "asg.h"

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

size_t parse_sid(const char *src, OoError *err, AsgSid *data) {
  Token t = tokenize(src);

  if (t.tt != ID) {
    err->tag = ERR_SID;
    err->tt = t.tt;
    return t.len;
  } else {
    err->tag = ERR_NONE;
  }

  data->src = src;
  data->len = t.len;

  return t.len;
}

void free_inner_id(AsgId data) {
  free(data.sids);
}

