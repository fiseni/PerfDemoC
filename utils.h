#ifndef COMMON_H
#define COMMON_H

#ifdef _MSC_VER
#define strdup _strdup
#endif

#include <stdbool.h>

bool is_suffix(const char* value, size_t lenValue, const char* source, size_t lenSource);
bool is_suffix_vectorized(const char* value, size_t lenValue, const char* source, size_t lenSource);
void to_upper_trim(const char* src, char* buffer, size_t bufferSize, size_t* outBufferLength);
void remove_char(const char* src, size_t srcLength, char* buffer, size_t bufferSize, char find, size_t* outBufferLength);

size_t next_power_of_two(size_t n);
#endif
