#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <immintrin.h>
#include "common.h"
#include "processor.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

void print_array(char* array[], size_t size) {
	for (size_t i = 0; i < size; i++) {
		printf("%s\n", array[i]);
	}
}

void print_masterParts(MasterPart* masterParts, size_t masterPartsCount) {
	for (size_t i = 0; i < masterPartsCount; i++) {
		printf("%s\n", masterParts[i].partNumber);
		printf("%s\n", masterParts[i].partNumberNoHyphens);
	}

	printf("#################################\n");
}

int compare_partNumber_length(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberLength;
	size_t lenB = ((const MasterPart*)b)->partNumberLength;

	// Compare lengths for ascending order
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

int compare_partNumber_length_desc(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberLength;
	size_t lenB = ((const MasterPart*)b)->partNumberLength;

	// Compare lengths for descending order
	return lenA < lenB ? 1 : lenA > lenB ? -1 : 0;
}

int compare_partNumberNoHyphens_length(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberNoHyphensLength;
	size_t lenB = ((const MasterPart*)b)->partNumberNoHyphensLength;

	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

void to_upper_trim(char* src, char* buffer, size_t bufferSize) {
	if (src == NULL || buffer == NULL || bufferSize == 0) {
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
}

void remove_char(char* src, char* buffer, size_t bufferSize, char find) {
	if (src == NULL || buffer == NULL || bufferSize == 0) {
		return;
	}

	size_t len = strlen(src);
	size_t j = 0;

	for (size_t i = 0; i < len && j < bufferSize - 1; i++) {
		if (src[i] != find) {
			buffer[j++] = (char)toupper((unsigned char)src[i]);
		}
	}
	buffer[j] = '\0';
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

	// The overhead of it for 8 bytes is not worth it. I didn't notice any performance improvement.
	// Process 8 bytes at a time using SSE2
	/*
	for (; i + 7 < length; i += 8) {
		// Load 8 bytes into the lower half of a 128-bit register
		__m128i chunk1 = _mm_loadl_epi64((const __m128i*)(s1 + i));
		__m128i chunk2 = _mm_loadl_epi64((const __m128i*)(s2 + i));

		// Compare the two chunks byte-wise
		__m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);

		// Create a mask from the comparison (only lower 8 bits are relevant)
		int mask = _mm_movemask_epi8(cmp) & 0xFF;

		if (mask != 0xFF) {
			return 1; // Strings differ
		}
	}
	*/

	// Compare any remaining bytes
	for (; i < length; i++) {
		if (s1[i] != s2[i]) {
			return 1;
		}
	}

	return 0; // Strings are equal
}

bool is_suffix(const char* value, size_t lenValue, const char* source, size_t lenSource) {
	if (lenValue > lenSource) {
		return false;
	}
	const char* endOfSource = source + (lenSource - lenValue);
	//return (strcmp(endOfSource, value) == 0);
	return (memcmp(endOfSource, value, lenValue) == 0);
}

bool is_suffix_vectorized(const char* value, size_t lenValue, const char* source, size_t lenSource) {
	if (lenValue > lenSource) {
		return false;
	}
	const char* endOfSource = source + (lenSource - lenValue);

	// For our use-case most of the time the strings are not equal.
	// It turns out checking the first char before vectorization improves the performance.
	return (endOfSource[0] == value[0] && strcmp_same_length_vectorized(endOfSource, value, lenValue) == 0);
}



MasterPart* build_masterParts(char* inputArray[], size_t inputSize, size_t minLen, size_t* outSize) {
	MasterPart* outputArray = malloc(inputSize * sizeof(*outputArray));
	if (!outputArray) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}

	size_t count = 0;
	for (size_t i = 0; i < inputSize; i++) {
		char* src = inputArray[i];

		char buffer1[MAX_LINE_LEN];
		to_upper_trim(src, buffer1, sizeof(buffer1));
		size_t buffer1_len = strlen(buffer1);

		if (buffer1_len >= minLen) {
			char buffer2[MAX_LINE_LEN];
			remove_char(buffer1, buffer2, sizeof(buffer2), '-');
			size_t buffer2_len = strlen(buffer2);

			outputArray[count].partNumberLength = (int)buffer1_len;
			outputArray[count].partNumberNoHyphensLength = (int)buffer2_len;
			outputArray[count].partNumber = malloc(buffer1_len + 1);
			outputArray[count].partNumberNoHyphens = malloc(buffer2_len + 1);

			if (!outputArray[count].partNumber || !outputArray[count].partNumberNoHyphens) {
				fprintf(stderr, "Memory allocation failed\n");
				exit(EXIT_FAILURE);
			}

			strcpy(outputArray[count].partNumber, buffer1);
			strcpy(outputArray[count].partNumberNoHyphens, buffer2);

			count++;
		}
	}

	*outSize = count;
	return outputArray;
}

char** read_file_lines(const char* filename, size_t* outLineCount) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	// First pass: count lines
	size_t lineCount = 0;
	char buffer[MAX_LINE_LEN];
	while (fgets(buffer, sizeof(buffer), file)) {
		lineCount++;
	}

	// Rewind file pointer to beginning
	rewind(file);

	// Allocate array of pointers for lines
	char** lines = malloc(lineCount * sizeof(*lines));
	if (!lines) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}

	// Second pass: read lines and store them
	for (size_t i = 0; i < lineCount; i++) {
		if (!fgets(buffer, sizeof(buffer), file)) {
			fprintf(stderr, "Failed to read line %zu\n", i);
			exit(EXIT_FAILURE);
		}

		//size_t len = strlen(buffer);
		//if (len > 0 && buffer[len - 1] == '\n')
		//{
		//	buffer[len - 1] = '\0';
		//}
		buffer[strcspn(buffer, "\r\n")] = '\0';

		lines[i] = strdup(buffer);
		if (!lines[i]) {
			fprintf(stderr, "Memory allocation failed\n");
			exit(EXIT_FAILURE);
		}
	}

	fclose(file);
	*outLineCount = lineCount;
	return lines;
}