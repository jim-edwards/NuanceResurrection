#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int strcpy_s(char *dest, size_t destsz, const char *src) {
  if (dest == NULL || src == NULL) {
    return -1; // Null pointer error
  }
  if (destsz == 0) {
    return -1; // Destination size is zero
  }

  size_t len = strlen(src);
  if (len >= destsz) {
    return -1; // Not enough space in destination buffer
  }

  // Perform the copy
  for (size_t i = 0; i <= len; i++) {
    dest[i] = src[i];
  }

  return 0; // Success
}


int strcat_s(char *dest, size_t destsz, const char *src) {
  if (dest == NULL || src == NULL) {
    return -1; // Null pointer error
  }
  if (destsz == 0) {
    return -1; // Destination size is zero
  }

  size_t dest_len = strlen(dest);  // Get the current length of the destination string
  size_t src_len = strlen(src);    // Get the length of the source string

  // Check if there is enough space in the destination buffer
  if (dest_len + src_len + 1 > destsz) {
    return -1; // Not enough space in destination buffer
  }

  // Concatenate the strings
  for (size_t i = 0; i <= src_len; i++) {
    dest[dest_len + i] = src[i];
  }

  return 0; // Success
}

int sprintf_s(char *dest, size_t destsz, const char *format, ...) {
#define STRSAFE_ERROR -1
#define STRSAFE_SUCCESS 0
  
  if (dest == NULL || format == NULL) {
    return STRSAFE_ERROR; // Null pointer error
  }
  if (destsz == 0) {
    return STRSAFE_ERROR; // Destination size is zero
  }

  va_list args;
  va_start(args, format);

  // Estimate the maximum length of the formatted string (this is a basic approach)
  // and check if it fits in the destination buffer. We rely on vsnprintf's return value.
  int required_size = vsnprintf(NULL, 0, format, args);  // Get the size needed
  va_end(args);

  if (required_size < 0 || required_size + 1 > destsz) {
    return STRSAFE_ERROR; // Not enough space or error in formatting
  }

  va_start(args, format);
  int written = vsnprintf(dest, destsz, format, args);
  va_end(args);

  if (written < 0 || written >= (int)destsz) {
    return STRSAFE_ERROR; // Error in copying or output exceeds buffer size
  }

  return STRSAFE_SUCCESS; // Success
}


