#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "hash_table.h"
#include "source_data.h"
#include "processor.h"

const size_t MAX_VALUE = ((size_t)-1);

const char* processor_get_identifier() {
	return "Processor4";
}

typedef struct PartsInfo {
	Part* parts;
	size_t partsCount;
	size_t startIndexByLengthDesc[MAX_LINE_LEN + 1];
} PartsInfo;

typedef struct MasterPartsInfo {
	MasterPart* masterParts;
	MasterPart* masterPartsNoHyphens;
	HashTable* suffixesByLength[MAX_LINE_LEN + 1];
	HashTable* suffixesByNoHyphensLength[MAX_LINE_LEN + 1];
	size_t masterPartsCount;
} MasterPartsInfo;;

HashTable* dictionary = NULL;
MasterPartsInfo* masterPartsInfo = NULL;
PartsInfo* partsInfo = NULL;

static void backward_fill(size_t* array) {
	size_t tmp = array[MAX_LINE_LEN];
	for (int i = (int)MAX_LINE_LEN; i >= 0; i--) {
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
	for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
		if (array[i] == MAX_VALUE) {
			array[i] = tmp;
		}
		else {
			tmp = array[i];
		}
	}
}

static int compare_part_length(const void* a, const void* b) {
	size_t lenA = ((const Part*)a)->partNumberLength;
	size_t lenB = ((const Part*)b)->partNumberLength;

	// Compare lengths for ascending order
	return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static MasterPartsInfo* build_masterPartsInfo(MasterPart* masterParts, size_t count) {
	MasterPart* masterPartsNoHyphens = malloc(count * sizeof(*masterPartsNoHyphens));
	if (!masterPartsNoHyphens) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(masterPartsNoHyphens, masterParts, count * sizeof(*masterPartsNoHyphens));

	qsort(masterParts, count, sizeof(*masterParts), compare_partNumber_length);
	qsort(masterPartsNoHyphens, count, sizeof(*masterPartsNoHyphens), compare_partNumberNoHyphens_length);

	MasterPartsInfo* mpInfo = malloc(sizeof(*mpInfo));
	if (!mpInfo) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	mpInfo->masterParts = masterParts;
	mpInfo->masterPartsNoHyphens = masterPartsNoHyphens;
	mpInfo->masterPartsCount = count;

	// Create start indices.
	size_t startIndexByLength[MAX_LINE_LEN + 1] = { 0 };
	size_t startIndexByLengthNoHyphens[MAX_LINE_LEN + 1] = { 0 };
	for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
		startIndexByLength[i] = MAX_VALUE;
		startIndexByLengthNoHyphens[i] = MAX_VALUE;
	}
	// Populate the start indices
	for (size_t i = 0; i < count; i++) {
		size_t length = masterParts[i].partNumberLength;
		if (startIndexByLength[length] == MAX_VALUE) {
			startIndexByLength[length] = i;
		}
		length = masterPartsNoHyphens[i].partNumberNoHyphensLength;
		if (startIndexByLengthNoHyphens[length] == MAX_VALUE) {
			startIndexByLengthNoHyphens[length] = i;
		}
	}
	backward_fill(startIndexByLength);
	backward_fill(startIndexByLengthNoHyphens);

	// Create hash tables
	for (size_t length = 0; length <= MAX_LINE_LEN; length++) {
		HashTable* table = htable_string_create();

		size_t startIndex = startIndexByLength[length];
		if (startIndex != MAX_VALUE) {
			for (size_t i = startIndex; i < count; i++) {
				MasterPart mp = masterParts[i];
				char* suffix = mp.partNumber + (mp.partNumberLength - length);
				htable_string_insert_if_not_exists(table, suffix, mp.partNumber);
			}
		}

		mpInfo->suffixesByLength[length] = table;
	}

	for (size_t length = 0; length <= MAX_LINE_LEN; length++) {
		HashTable* table = htable_string_create();

		size_t startIndex = startIndexByLengthNoHyphens[length];
		if (startIndex != MAX_VALUE) {
			for (size_t i = startIndex; i < count; i++) {
				MasterPart mp = masterPartsNoHyphens[i];
				char* suffix = mp.partNumberNoHyphens + (mp.partNumberNoHyphensLength - length);
				htable_string_insert_if_not_exists(table, suffix, mp.partNumber);
			}
		}

		mpInfo->suffixesByNoHyphensLength[length] = table;
	}

	return mpInfo;
}

static PartsInfo* build_partsInfo(Part* inputArray, size_t inputSize, size_t minLen) {
	Part* parts = malloc(inputSize * sizeof(*parts));
	if (!parts) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}

	size_t count = 0;
	for (size_t i = 0; i < inputSize; i++) {
		char* src = inputArray[i].partNumber;

		char buffer[MAX_LINE_LEN];
		to_upper_trim(src, buffer, sizeof(buffer));
		size_t buffer_len = strlen(buffer);

		if (buffer_len >= minLen) {
			Part* part = &parts[count];
			part->partNumberLength = (int)buffer_len;
			part->partNumber = malloc(buffer_len + 1);
			if (!part->partNumber) {
				fprintf(stderr, "Memory allocation failed\n");
				exit(EXIT_FAILURE);
			}
			strcpy(part->partNumber, buffer);
			count++;
		}
	}
	qsort(parts, count, sizeof(*parts), compare_part_length);

	PartsInfo* partsInfo = malloc(sizeof(*partsInfo));
	if (!partsInfo) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	partsInfo->parts = parts;
	partsInfo->partsCount = count;

	// Populate the start indices
	for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
		partsInfo->startIndexByLengthDesc[i] = MAX_VALUE;
	}
	for (size_t i = 0; i < masterPartsInfo->masterPartsCount; i++) {
		size_t length = masterPartsInfo->masterParts[i].partNumberLength;
		partsInfo->startIndexByLengthDesc[length] = i;
	}
	forward_fill(partsInfo->startIndexByLengthDesc);

	return partsInfo;
}

void processor_initialize(SourceData* data) {
	masterPartsInfo = build_masterPartsInfo(data->masterParts, data->masterPartsCount);
	partsInfo = build_partsInfo(data->parts, data->partsCount, 3);

	dictionary = htable_string_create();

	for (size_t i = 0; i < partsInfo->partsCount; i++) {
		Part* part = &partsInfo->parts[i];

		HashTable* masterPartsBySuffix = masterPartsInfo->suffixesByLength[part->partNumberLength];
		if (masterPartsBySuffix) {
			const char* match = htable_string_search(masterPartsBySuffix, part->partNumber);
			if (match) {
				htable_string_insert_if_not_exists(dictionary, part->partNumber, match);
			}
		}
		masterPartsBySuffix = masterPartsInfo->suffixesByNoHyphensLength[part->partNumberLength];
		if (masterPartsBySuffix) {
			const char* match = htable_string_search(masterPartsBySuffix, part->partNumber);
			if (match) {
				htable_string_insert_if_not_exists(dictionary, part->partNumber, match);
			}
		}
	}
}

const char* processor_find_match(char* partNumber) {

	if (partNumber == NULL) {
		return NULL;
	}

	char buffer[MAX_LINE_LEN];
	to_upper_trim(partNumber, buffer, sizeof(buffer));
	size_t bufferLen = strlen(buffer);
	if (bufferLen < 3) {
		return NULL;
	}

	const char* match = htable_string_search(dictionary, buffer);
	if (match) {
		return match;
	}

	size_t startIndex = partsInfo->startIndexByLengthDesc[bufferLen];
	if (startIndex != MAX_VALUE) {
		for (long i = (long)startIndex; i >= 0; i--) {
			MasterPart mp = masterPartsInfo->masterParts[i];
			if (is_suffix_vectorized(mp.partNumber, mp.partNumberLength, buffer, bufferLen)) {
				return mp.partNumber;
			}
		}
	}

	return NULL;
}

void processor_clean() {

}