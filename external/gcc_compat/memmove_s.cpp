#include <stddef.h>
#include <string.h>
#include <errno.h>

int memmove_s(void* dest, size_t destSize, const void* src, size_t count) {
  if (!dest || !src) {
    return EINVAL;  // Invalid argument (NULL pointer)
  }

  if (count > destSize) {
    return ERANGE;  // Buffer too small
  }

  memmove(dest, src, count);
  return 0;  // Success
}