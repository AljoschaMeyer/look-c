#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool str_eq(const char *s1, size_t len1, const char *s2, size_t len2) {
  if (len1 != len2) {
    return false;
  } else {
    return memcmp(s1, s2, len1) == 0;
  }
}
