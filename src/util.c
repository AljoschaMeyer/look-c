#include "util.h"

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
