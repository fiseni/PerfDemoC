#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <immintrin.h>
#include "utils.h"

void to_upper_trim(const char* src, char* buffer, size_t bufferSize, size_t* outBufferLength) {
    if (src == NULL || buffer == NULL || bufferSize == 0) {
        *outBufferLength = 0;
        return;
    }
    size_t len = strlen(src);
    size_t j = 0;
    for (size_t i = 0; i < len && j < bufferSize - 1; i++) {
        if (!isspace((unsigned char)src[i])) {
            buffer[j++] = (char)toupper((unsigned char)src[i]);
        }
    }
    buffer[j] = '\0';
    *outBufferLength = j;
}

void remove_char(const char* src, size_t srcLength, char* buffer, size_t bufferSize, char find, size_t* outBufferLength) {
    if (src == NULL || buffer == NULL || bufferSize == 0) {
        *outBufferLength = 0;
        return;
    }
    size_t j = 0;
    for (size_t i = 0; i < srcLength && j < bufferSize - 1; i++) {
        if (src[i] != find) {
            buffer[j++] = (char)toupper((unsigned char)src[i]);
        }
    }
    buffer[j] = '\0';
    *outBufferLength = j;
}

static int strcmp_same_length_vectorized(const char* s1, const char* s2, size_t length) {
    size_t i = 0;

    // Process 32 bytes at a time using AVX2
    // There are very few cases where strings are longer than 32 bytes. Not worth it.
    /*
    for (; i + 31 < length; i += 32) {
        // Load 32 bytes from each string (unaligned load)
        __m256i chunk1 = _mm256_loadu_si256((const __m256i*)(s1 + i));
        __m256i chunk2 = _mm256_loadu_si256((const __m256i*)(s2 + i));

        // Compare the two chunks byte-wise
        __m256i cmp = _mm256_cmpeq_epi8(chunk1, chunk2);

        // Create a mask from the comparison
        int mask = _mm256_movemask_epi8(cmp);

        // If mask is not all ones, there's a difference
        if (mask != 0xFFFFFFFF) {
            return 1;
        }
    }
    */

    // Process 16 bytes at a time using SSE2
    for (; i + 15 < length; i += 16) {
        // Load 16 bytes from each string (unaligned load)
        __m128i chunk1 = _mm_loadu_si128((const __m128i*)(s1 + i));
        __m128i chunk2 = _mm_loadu_si128((const __m128i*)(s2 + i));

        __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
        int mask = _mm_movemask_epi8(cmp);

        if (mask != 0xFFFF) {
            return 1;
        }
    }

    // Compare any remaining bytes
    for (; i < length; i++) {
        if (s1[i] != s2[i]) {
            return 1;
        }
    }

    return 0; // Strings are equal
}

bool is_suffix(const char* value, size_t valueLength, const char* source, size_t sourceLength) {
    if (valueLength > sourceLength) {
        return false;
    }
    const char* endOfSource = source + (sourceLength - valueLength);
    //return (strcmp(endOfSource, value) == 0);
    return (memcmp(endOfSource, value, valueLength) == 0);
}

bool is_suffix_vectorized(const char* value, size_t valueLength, const char* source, size_t sourceLength) {
    if (valueLength > sourceLength) {
        return false;
    }
    const char* endOfSource = source + (sourceLength - valueLength);

    // For our use-case most of the time the strings are not equal.
    // It turns out checking the first char before vectorization improves the performance.
    return (endOfSource[0] == value[0] && strcmp_same_length_vectorized(endOfSource, value, valueLength) == 0);
}

static int is_power_of_two(size_t n) {
    if (n == 0)
        return 0;
    return (n & (n - 1)) == 0;
}

size_t next_power_of_two(size_t n) {
    if (n == 0)
        return 1;
    if (is_power_of_two(n))
        return n;

    // Subtract 1 to ensure correct bit setting for the next power of 2
    n--;
    int bits = sizeof(size_t) * CHAR_BIT;

    // Set all bits to the right of the MSB
    for (int shift = 1; shift < bits; shift <<= 1) {
        n |= n >> shift;
    }

    // Add 1 to get the next power of 2
    n++;

    // For clarity in case of overflow.
    // If n becomes 0 after shifting, it means the next power of 2 exceeds the limit
    if (n == 0)
        return 0;
    return n;
}
