#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "source_data.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

static MasterPart* build_masterParts(char* inputArray[], size_t inputSize, size_t minLen, size_t* outSize) {
	MasterPart* outputArray = malloc(inputSize * sizeof(*outputArray));
	assert(outputArray);

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

			assert(outputArray[count].partNumber && outputArray[count].partNumberNoHyphens);

			strcpy(outputArray[count].partNumber, buffer1);
			strcpy(outputArray[count].partNumberNoHyphens, buffer2);

			count++;
		}
	}

	*outSize = count;
	return outputArray;
}

static char** read_file_lines(const char* filename, size_t* outLineCount) {
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
	assert(lines);

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
		assert(lines[i]);
	}

	fclose(file);
	*outLineCount = lineCount;
	return lines;
}

SourceData* data_build(char** masterPartNumbers, size_t masterPartNumbersCount, char** partNumbers, size_t partNumbersCount) 	{
	size_t masterPartsCount = 0;
	MasterPart* masterParts = build_masterParts(masterPartNumbers, masterPartNumbersCount, 3, &masterPartsCount);

	SourceData* data = (SourceData*)malloc(sizeof(*data));
	assert(data);

	Part* parts = malloc(partNumbersCount * sizeof(*parts));
	assert(parts);

	for (size_t i = 0; i < partNumbersCount; i++) {
		parts[i].partNumber = partNumbers[i];
		parts[i].partNumberLength = (int)strlen(partNumbers[i]);
	}

	data->masterParts = masterParts;
	data->masterPartsCount = masterPartsCount;
	data->parts = parts;
	data->partsCount = partNumbersCount;

	return data;
}

SourceData* data_read(int argc, char* argv[]) {
	char* partFile = "data/parts.txt";
	char* masterPartFile = "data/masterParts.txt";
	//partFile = "data/partsTest.txt";
	//masterPartFile = "data/masterPartsTest.txt";

	if (argc > 1 && strcmp(argv[1], "short") == 0) {
		partFile = "data/partsShort.txt";
		masterPartFile = "data/masterPartsShort.txt";
	}
	size_t partNumbersCount = 0;
	char** partNumbers = read_file_lines(partFile, &partNumbersCount);
	size_t masterPartNumbersCount = 0;
	char** masterPartNumbers = read_file_lines(masterPartFile, &masterPartNumbersCount);

	return data_build(masterPartNumbers, masterPartNumbersCount, partNumbers, partNumbersCount);
}

void data_print(SourceData* data) {
	for (size_t i = 0; i < data->masterPartsCount; i++) {
		printf("%s\n", data->masterParts[i].partNumber);
		printf("%s\n", data->masterParts[i].partNumberNoHyphens);
	}

	printf("#################################\n");
}

int compare_mp_by_partNumber_length_asc(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberLength;
	size_t lenB = ((const MasterPart*)b)->partNumberLength;
	// Compare lengths for ascending order
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

int compare_mp_by_partNumber_length_desc(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberLength;
	size_t lenB = ((const MasterPart*)b)->partNumberLength;
	// Compare lengths for descending order
	return lenA < lenB ? 1 : lenA > lenB ? -1 : 0;
}

int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b) {
	size_t lenA = ((const MasterPart*)a)->partNumberNoHyphensLength;
	size_t lenB = ((const MasterPart*)b)->partNumberNoHyphensLength;
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

int compare_part_by_partNumber_length_asc(const void* a, const void* b) {
	size_t lenA = ((const Part*)a)->partNumberLength;
	size_t lenB = ((const Part*)b)->partNumberLength;

	// Compare lengths for ascending order
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}