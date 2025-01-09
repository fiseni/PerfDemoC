#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cross_platform_time.h"
#include "common.h"
#include "processor.h"

MasterPart* masterPartsAsc = NULL;
MasterPart* masterPartsDesc = NULL;
MasterPart* masterPartsAscByNoHyphens = NULL;
size_t masterPartsCount = 0;
char** parts = NULL;
size_t partsCount = 0;

void initialize(MasterPart* masterParts, size_t count, char** partNumbers, size_t partNumbersCount) {
	masterPartsAsc = masterParts;
	masterPartsCount = count;
	parts = partNumbers;
	partsCount = partNumbersCount;

	masterPartsDesc = malloc(masterPartsCount * sizeof(*masterPartsDesc));
	if (!masterPartsDesc) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(masterPartsDesc, masterPartsAsc, masterPartsCount * sizeof(*masterPartsDesc));

	masterPartsAscByNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
	if (!masterPartsAscByNoHyphens) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));

	qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_partNumber_length);
	qsort(masterPartsDesc, masterPartsCount, sizeof(*masterPartsDesc), compare_partNumber_length_desc);
	qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_partNumberNoHyphens_length);

	//print_masterParts(masterPartsAsc, masterPartsCount);
	//print_masterParts(masterPartsDesc, masterPartsCount);
	//print_masterParts(masterPartsAscByNoHyphens, masterPartsCount);
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

	for (size_t i = 0; i < masterPartsCount; i++) {
		MasterPart mp = masterPartsAsc[i];
		if (is_suffix_vectorized(buffer, bufferLen, mp.PartNumber, strlen(mp.PartNumber))) {
			return mp.PartNumber;
		}
	}

	for (size_t i = 0; i < masterPartsCount; i++) {
		MasterPart mp = masterPartsAscByNoHyphens[i];
		if (is_suffix_vectorized(buffer, bufferLen, mp.PartNumberNoHyphens, strlen(mp.PartNumberNoHyphens))) {
			return mp.PartNumber;
		}
	}

	for (size_t i = 0; i < masterPartsCount; i++) {
		MasterPart mp = masterPartsDesc[i];
		if (is_suffix_vectorized(mp.PartNumber, strlen(mp.PartNumber), buffer, bufferLen)) {
			return mp.PartNumber;
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

	if (masterPartsDesc) {
		free(masterPartsDesc);
		masterPartsDesc = NULL;
	}

	if (masterPartsAscByNoHyphens) {
		free(masterPartsAscByNoHyphens);
		masterPartsAscByNoHyphens = NULL;
	}

	return matchCount;
}