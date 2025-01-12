#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"
#include "source_data.h"
#include "cross_platform_time.h"
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
	HTableString* suffixesByLength[MAX_LINE_LEN + 1];
	HTableString* suffixesByNoHyphensLength[MAX_LINE_LEN + 1];
	size_t masterPartsCount;
} MasterPartsInfo;;

HTableString* dictionary = NULL;
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

static MasterPartsInfo* build_masterPartsInfo(MasterPart* masterParts, size_t masterPartsCount) {
	qsort(masterParts, masterPartsCount, sizeof(*masterParts), compare_mp_by_partNumber_length_asc);
	MasterPart* masterPartsNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsNoHyphens));
	assert(masterPartsNoHyphens);
	memcpy(masterPartsNoHyphens, masterParts, masterPartsCount * sizeof(*masterPartsNoHyphens));
	qsort(masterPartsNoHyphens, masterPartsCount, sizeof(*masterPartsNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

	MasterPartsInfo* mpInfo = malloc(sizeof(*mpInfo));
	assert(mpInfo);
	mpInfo->masterParts = masterParts;
	mpInfo->masterPartsNoHyphens = masterPartsNoHyphens;
	mpInfo->masterPartsCount = masterPartsCount;

	// Create and populate start indices.
	size_t startIndexByLength[MAX_LINE_LEN + 1] = { 0 };
	size_t startIndexByLengthNoHyphens[MAX_LINE_LEN + 1] = { 0 };
	for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
		startIndexByLength[i] = MAX_VALUE;
		startIndexByLengthNoHyphens[i] = MAX_VALUE;
	}
	for (size_t i = 0; i < masterPartsCount; i++) {
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
	for (size_t length = 3; length <= MAX_LINE_LEN; length++) {
		HTableString* table = NULL;
		size_t startIndex = startIndexByLength[length];
		if (startIndex != MAX_VALUE) {
			if (!table) {
				table = htable_string_create();
			}
			for (size_t i = startIndex; i < masterPartsCount; i++) {
				MasterPart mp = masterParts[i];
				char* suffix = mp.partNumber + (mp.partNumberLength - length);
				htable_string_insert_if_not_exists(table, suffix, length, mp.partNumber);
			}
		}
		mpInfo->suffixesByLength[length] = table;
	}
	for (size_t length = 3; length <= MAX_LINE_LEN; length++) {
		HTableString* table = NULL;
		size_t startIndex = startIndexByLengthNoHyphens[length];
		if (startIndex != MAX_VALUE) {
			if (!table) {
				table = htable_string_create();
			}
			for (size_t i = startIndex; i < masterPartsCount; i++) {
				MasterPart mp = masterPartsNoHyphens[i];
				char* suffix = mp.partNumberNoHyphens + (mp.partNumberNoHyphensLength - length);
				htable_string_insert_if_not_exists(table, suffix, length, mp.partNumber);
			}
		}
		mpInfo->suffixesByNoHyphensLength[length] = table;
	}

	return mpInfo;
}

static PartsInfo* build_partsInfo(Part* inputArray, size_t inputSize, size_t minLen) {
	Part* parts = malloc(inputSize * sizeof(*parts));
	assert(parts);

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
			assert(part->partNumber);
			strcpy(part->partNumber, buffer);
			count++;
		}
	}
	qsort(parts, count, sizeof(*parts), compare_part_by_partNumber_length_asc);

	PartsInfo* partsInfo = malloc(sizeof(*partsInfo));
	assert(partsInfo);
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
		Part part = partsInfo->parts[i];

		HTableString* masterPartsBySuffix = masterPartsInfo->suffixesByLength[part.partNumberLength];
		if (masterPartsBySuffix) {
			const char* match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
			if (match) {
				htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
			}
		}
		masterPartsBySuffix = masterPartsInfo->suffixesByNoHyphensLength[part.partNumberLength];
		if (masterPartsBySuffix) {
			const char* match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
			if (match) {
				htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
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

	const char* match = htable_string_search(dictionary, buffer, bufferLen);
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