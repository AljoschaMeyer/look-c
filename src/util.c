#include <stdio.h>

#include "util.h"

Str str_new(const char *start, size_t len) {
  Str s;
  s.start = start;
  s.len = len;
  return s;
}

void str_print(Str s) {
  fwrite(s.start, 1, s.len, stdout);
  printf("\n");
}

bool str_eq(Str s1, Str s2) {
  if (s1.len != s2.len) {
    return false;
  } else {
    return memcmp(s1.start, s2.start, s1.len) == 0;
  }
}

bool str_eq_parts(Str s, const char *chars, size_t len) {
  Str other;
  other.start = chars;
  other.len = len;
  return str_eq(s, other);
}
