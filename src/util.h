#ifndef OO_UTIL_H
#define OO_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct Str {
  const char *start;
  size_t len;
} Str;

bool str_eq(Str s1, Str s2);
bool str_eq_parts(Str s, const char *chars, size_t len);

#endif
