#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "source_data.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

static MasterPart* build_masterParts(char* inputArray[], size_t inputSize, size_t minLen, size_t* outSize) {
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

SourceData* data_build(char** masterPartNumbers, size_t masterPartNumbersCount, char** partNumbers, size_t partNumbersCount) 	{
	size_t masterPartsCount = 0;
	MasterPart* masterParts = build_masterParts(masterPartNumbers, masterPartNumbersCount, 3, &masterPartsCount);

	SourceData* data = (SourceData*)malloc(sizeof(*data));
	if (!data) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	data->masterParts = masterParts;
	data->masterPartsCount = masterPartsCount;
	data->partNumbers = partNumbers;
	data->partNumbersCount = partNumbersCount;

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

void data_clean(SourceData* data) {
	// We don't really need to clean anything here. We're just trying to mimic the actions in the real app.
	for (size_t i = 0; i < data->masterPartsCount; i++) {
		free(data->masterParts[i].partNumber);
		data->masterParts[i].partNumber = NULL;
		free(data->masterParts[i].partNumberNoHyphens);
		data->masterParts[i].partNumberNoHyphens = NULL;
	}
	free(data->masterParts);
	data->masterParts = NULL;

	//for (size_t i = 0; i < data->masterPartNumbersCount; i++) {
	//	free(data->masterPartNumbers[i]);
	//	data->masterPartNumbers[i] = NULL;
	//}
	//for (size_t i = 0; i < data->partNumbersCount; i++) {
	//	free(data->partNumbers[i]);
	//	data->partNumbers[i] = NULL;
	//}
	//free(data->masterPartNumbers);
	//data->masterPartNumbers = NULL;
	//free(data->partNumbers);
	//data->partNumbers = NULL;
}

void data_print(SourceData* data) {
	for (size_t i = 0; i < data->masterPartsCount; i++) {
		printf("%s\n", data->masterParts[i].partNumber);
		printf("%s\n", data->masterParts[i].partNumberNoHyphens);
	}

	printf("#################################\n");
}