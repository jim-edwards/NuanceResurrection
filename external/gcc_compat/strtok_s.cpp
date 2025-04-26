#include <cstring>  // For strtok
#include <cstdio>   // For null terminator and printf (if needed)

char* strtok_s(char* str, const char* delim, char** context) {
  if (str == nullptr) {
    str = *context;
  }

  // Use strtok to get the token
  char* token = strtok(str, delim);

  // Save the context for the next call
  *context = nullptr; // If `str` is null, strtok keeps state internally.
  return token;
}