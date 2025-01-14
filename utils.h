#ifndef COMMON_H
#define COMMON_H

#ifdef _MSC_VER
#define strdup _strdup
#endif

// Macro to check allocation
#define CHECK_ALLOC(ptr)                                   \
    do {                                                   \
        if (!(ptr)) {                                      \
            fprintf(stderr, "Memory allocation failed\n"); \
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)

#include <stdbool.h>

bool is_suffix(const char* value, size_t valueLength, const char* source, size_t sourceLength);
bool is_suffix_vectorized(const char* value, size_t valueLength, const char* source, size_t sourceLength);
void to_upper_trim(const char* src, char* buffer, size_t bufferSize, size_t* outBufferLength);
void remove_char(const char* src, size_t srcLength, char* buffer, size_t bufferSize, char find, size_t* outBufferLength);

size_t next_power_of_two(size_t n);
#endif
