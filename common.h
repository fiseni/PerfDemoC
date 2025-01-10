#ifndef COMMON_H
#define COMMON_H

// It's provided that the length of part numbers is maximum 50 characters
#define MAX_LINE_LEN ((size_t)50)

#include <stdlib.h>
#include <stdbool.h>

typedef struct MasterPart {
	int partNumberLength;
	int partNumberNoHyphensLength;
	char* partNumber;
	char* partNumberNoHyphens;
} MasterPart;

void run_tests();

char** read_file_lines(const char* filename, size_t* outLineCount);
MasterPart* build_masterParts(char* inputArray[], size_t inputSize, size_t minLen, size_t* outSize);

bool is_suffix(const char* value, size_t lenValue, const char* source, size_t lenSource);
bool is_suffix_vectorized(const char* value, size_t lenValue, const char* source, size_t lenSource);

int compare_partNumber_length(const void* a, const void* b);
int compare_partNumber_length_desc(const void* a, const void* b);
int compare_partNumberNoHyphens_length(const void* a, const void* b);

void to_upper_trim(char* src, char* buffer, size_t bufferSize);
void remove_char(char* src, char* buffer, size_t bufferSize, char find);

void print_array(char* array[], size_t count);
void print_masterParts(MasterPart* masterParts, size_t masterPartsCount);

#endif