#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cross_platform_time.h"
#include "common.h"
#include "processor.h"

const size_t MAX_VALUE = ((size_t)-1);

const char* get_identifier() {
	return "Processor2";
}

MasterPart* masterPartsAsc = NULL;
MasterPart* masterPartsAscByNoHyphens = NULL;
size_t masterPartsCount = 0;
size_t startIndexByLengthAsc[MAX_LINE_LEN + 1];
size_t startIndexByLengthAscNoHyphens[MAX_LINE_LEN + 1];
size_t startIndexByLengthDesc[MAX_LINE_LEN + 1];

char** parts = NULL;
size_t partsCount = 0;

static void backward_fill(size_t* array) {
	size_t tmp = array[MAX_LINE_LEN];
	for (int i = MAX_LINE_LEN; i >= 0; i--) {
		if (array[i] == MAX_VALUE) {
			array[i] = tmp;
		}
		else {
			tmp = array[i];
		}
	}
}

static void forward_fill(size_t* array) {
	size_t tmp = array[0];
	for (int i = 0; i <= MAX_LINE_LEN; i++) {
		if (array[i] == MAX_VALUE) {
			array[i] = tmp;
		}
		else {
			tmp = array[i];
		}
	}
}

void initialize(MasterPart* masterParts, size_t count, char** partNumbers, size_t partNumbersCount) {
	masterPartsAsc = masterParts;
	masterPartsCount = count;
	parts = partNumbers;
	partsCount = partNumbersCount;

	masterPartsAscByNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
	if (!masterPartsAscByNoHyphens) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));

	qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_partNumber_length);
	qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_partNumberNoHyphens_length);


	for (int i = 0; i <= MAX_LINE_LEN; i++) {
		startIndexByLengthAsc[i] = MAX_VALUE;
		startIndexByLengthAscNoHyphens[i] = MAX_VALUE;
		startIndexByLengthDesc[i] = MAX_VALUE;
	}

	// Populate the start indices
	for (size_t i = 0; i < masterPartsCount; i++) {
		size_t length = masterPartsAsc[i].PartNumberLength;
		if (startIndexByLengthAsc[length] == MAX_VALUE) {
			startIndexByLengthAsc[length] = i;
		}
		startIndexByLengthDesc[length] = i;

		length = masterPartsAscByNoHyphens[i].PartNumberNoHyphensLength;
		if (startIndexByLengthAscNoHyphens[length] == MAX_VALUE) {
			startIndexByLengthAscNoHyphens[length] = i;
		}
	}

	backward_fill(startIndexByLengthAsc);
	backward_fill(startIndexByLengthAscNoHyphens);
	forward_fill(startIndexByLengthDesc);
}

char* find_match(char* partNumber) {

	if (partNumber == NULL) {
		return NULL;
	}

	char buffer[MAX_LINE_LEN];
	to_upper_trim(partNumber, buffer, sizeof(buffer));
	size_t bufferLen = strlen(buffer);
	if (bufferLen < 3) {
		return NULL;
	}

	size_t startIndex = startIndexByLengthAsc[bufferLen];
	if (startIndex != MAX_VALUE) {
		for (size_t i = startIndex; i < masterPartsCount; i++) {
			MasterPart mp = masterPartsAsc[i];
			if (is_suffix_vectorized(buffer, bufferLen, mp.PartNumber, mp.PartNumberLength)) {
				return mp.PartNumber;
			}
		}
	}

	startIndex = startIndexByLengthAscNoHyphens[bufferLen];
	if (startIndex != MAX_VALUE) {
		for (size_t i = startIndex; i < masterPartsCount; i++) {
			MasterPart mp = masterPartsAscByNoHyphens[i];
			if (is_suffix_vectorized(buffer, bufferLen, mp.PartNumberNoHyphens, mp.PartNumberNoHyphensLength)) {
				return mp.PartNumber;
			}
		}
	}

	startIndex = startIndexByLengthDesc[bufferLen];
	if (startIndex != MAX_VALUE) {
		for (long i = (long)startIndex; i >= 0; i--) {
			MasterPart mp = masterPartsAsc[i];
			if (is_suffix_vectorized(mp.PartNumber, mp.PartNumberLength, buffer, bufferLen)) {
				return mp.PartNumber;
			}
		}
	}

	return NULL;
}

size_t run() {

	size_t matchCount = 0;
	char* result = NULL;

	for (size_t i = 0; i < partsCount; i++) {
		result = find_match(parts[i]);

		if (result) {
			matchCount++;
		}

		//printf("PartNumber: %30s %30s\n", partNumbers[i], result);
	};

	if (masterPartsAscByNoHyphens) {
		free(masterPartsAscByNoHyphens);
		masterPartsAscByNoHyphens = NULL;
	}

	return matchCount;
}