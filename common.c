#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <emmintrin.h>
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
		printf("%s\n", masterParts[i].PartNumber);
		printf("%s\n", masterParts[i].PartNumberNoHyphens);
	}

	printf("#################################\n");
}

int compare_partNumber_length(const void* a, const void* b) {
	const MasterPart* mpA = (const MasterPart*)a;
	const MasterPart* mpB = (const MasterPart*)b;

	size_t lenA = strlen(mpA->PartNumber);
	size_t lenB = strlen(mpB->PartNumber);

	// Compare lengths for ascending order
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

int compare_partNumber_length_desc(const void* a, const void* b) {
	const MasterPart* mpA = (const MasterPart*)a;
	const MasterPart* mpB = (const MasterPart*)b;

	size_t lenA = strlen(mpA->PartNumber);
	size_t lenB = strlen(mpB->PartNumber);

	// Compare lengths for descending order
	return lenA < lenB ? 1 : lenA > lenB ? -1 : 0;
}

int compare_partNumberNoHyphens_length(const void* a, const void* b) {
	const MasterPart* mpA = (const MasterPart*)a;
	const MasterPart* mpB = (const MasterPart*)b;

	size_t lenA = strlen(mpA->PartNumberNoHyphens);
	size_t lenB = strlen(mpB->PartNumberNoHyphens);

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

static int vectorized_strcmp_same_length(const char* s1, const char* s2, size_t length) {
	size_t i = 0;

	// Process 16 bytes at a time using SSE2
	for (; i + 15 < length; i += 16) {
		// Load 16 bytes from each string
		__m128i chunk1 = _mm_loadu_si128((const __m128i*)(s1 + i));
		__m128i chunk2 = _mm_loadu_si128((const __m128i*)(s2 + i));

		// Compare the two chunks
		__m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);

		// Create a mask from the comparison
		int mask = _mm_movemask_epi8(cmp);

		// If mask is not all ones, there's a difference
		if (mask != 0xFFFF) {
			return 1;  // Strings differ
		}
	}

	// Compare any remaining bytes
	for (; i < length; i++) {
		if (s1[i] != s2[i]) {
			return 1;  // Strings differ
		}
	}

	return 0;  // Strings are equal
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

	// Most of the time the strings are not equal. It turns out checking the first char improves the performance.
	return (endOfSource[0] == value[0] && vectorized_strcmp_same_length(endOfSource, value, lenValue) == 0);
}

MasterPart* build_masterParts(char* inputArray[], size_t inputSize, size_t minLen, size_t* outputSize) {
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

			outputArray[count].PartNumber = malloc(buffer1_len + 1);
			outputArray[count].PartNumberNoHyphens = malloc(strlen(buffer2) + 1);

			if (!outputArray[count].PartNumber || !outputArray[count].PartNumberNoHyphens) {
				fprintf(stderr, "Memory allocation failed\n");
				exit(EXIT_FAILURE);
			}

			strcpy(outputArray[count].PartNumber, buffer1);
			strcpy(outputArray[count].PartNumberNoHyphens, buffer2);

			count++;
		}
	}

	*outputSize = count;
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