#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

bool is_suffix(const char* value, size_t lenValue, const char* source, size_t lenSource);
bool is_suffix_vectorized(const char* value, size_t lenValue, const char* source, size_t lenSource);
void to_upper_trim(char* src, char* buffer, size_t bufferSize);
void remove_char(char* src, char* buffer, size_t bufferSize, char find);

#endif
