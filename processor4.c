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
    HTableSizeList* suffixesByLength[MAX_STRING_LENGTH];
} PartsInfo;

typedef struct MasterPartsInfo {
    MasterPart* masterParts;
    MasterPart* masterPartsNoHyphens;
    size_t masterPartsCount;
    size_t masterPartsNoHyphensCount;
    HTableString* suffixesByLength[MAX_STRING_LENGTH];
    HTableString* suffixesByNoHyphensLength[MAX_STRING_LENGTH];
} MasterPartsInfo;;

HTableString* dictionary = NULL;
MasterPartsInfo* masterPartsInfo = NULL;
PartsInfo* partsInfo = NULL;

static void backward_fill(size_t* array) {
    size_t tmp = array[MAX_STRING_LENGTH - 1];
    for (long length = (long)MAX_STRING_LENGTH - 1; length >= 0; length--) {
        if (array[length] == MAX_VALUE) {
            array[length] = tmp;
        }
        else {
            tmp = array[length];
        }
    }
}

static bool containsDash(const char* str, size_t strLength) {
    for (size_t i = 0; i < strLength; i++) {
        if (str[i] == '-') {
            return true;
        }
    }
    return false;
}

static MasterPart* build_masterPartsNyHyphen(const MasterPart* masterParts, size_t masterPartsCount, size_t* outCount) {
    MasterPart* masterPartsNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsNoHyphens));
    CHECK_ALLOC(masterPartsNoHyphens);
    size_t count = 0;
    for (size_t i = 0; i < masterPartsCount; i++) {
        if (containsDash(masterParts[i].partNumber, masterParts[i].partNumberLength)) {
            masterPartsNoHyphens[count++] = masterParts[i];
        }
        //if (strchr(masterParts[i].partNumber, '-')) {
        //	masterPartsNoHyphens[count++] = masterParts[i];
        //}
    }
    qsort(masterPartsNoHyphens, count, sizeof(*masterPartsNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);
    *outCount = count;
    return masterPartsNoHyphens;
}

static MasterPartsInfo* build_masterPartsInfo(const MasterPart* inputArray, size_t inputArrayCount) {
    size_t masterPartsCount = inputArrayCount;
    MasterPart* masterParts = malloc(masterPartsCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);
    memcpy(masterParts, inputArray, masterPartsCount * sizeof(*masterParts));
    qsort(masterParts, masterPartsCount, sizeof(*masterParts), compare_mp_by_partNumber_length_asc);
    size_t masterPartsNoHyphensCount = 0;
    MasterPart* masterPartsNoHyphens = build_masterPartsNyHyphen(masterParts, masterPartsCount, &masterPartsNoHyphensCount);

    MasterPartsInfo* mpInfo = malloc(sizeof(*mpInfo));
    CHECK_ALLOC(mpInfo);
    mpInfo->masterParts = masterParts;
    mpInfo->masterPartsNoHyphens = masterPartsNoHyphens;
    mpInfo->masterPartsCount = masterPartsCount;
    mpInfo->masterPartsNoHyphensCount = masterPartsNoHyphensCount;

    // Create and populate start indices.
    size_t startIndexByLength[MAX_STRING_LENGTH] = { 0 };
    size_t startIndexByLengthNoHyphens[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLength[length] = MAX_VALUE;
        startIndexByLengthNoHyphens[length] = MAX_VALUE;
    }
    for (size_t i = 0; i < masterPartsCount; i++) {
        size_t length = masterParts[i].partNumberLength;
        if (startIndexByLength[length] == MAX_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    for (size_t i = 0; i < masterPartsNoHyphensCount; i++) {
        size_t length = masterPartsNoHyphens[i].partNumberNoHyphensLength;
        if (startIndexByLengthNoHyphens[length] == MAX_VALUE) {
            startIndexByLengthNoHyphens[length] = i;
        }
    }
    backward_fill(startIndexByLength);
    backward_fill(startIndexByLengthNoHyphens);

    // Create hash tables
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        mpInfo->suffixesByLength[length] = NULL;
        mpInfo->suffixesByNoHyphensLength[length] = NULL;
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        HTableString* table = NULL;
        size_t startIndex = startIndexByLength[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_string_create(masterPartsCount);
            }
            for (size_t i = startIndex; i < masterPartsCount; i++) {
                MasterPart mp = masterParts[i];
                const char* suffix = mp.partNumber + (mp.partNumberLength - length);
                htable_string_insert_if_not_exists(table, suffix, length, mp.partNumber);
            }
        }
        mpInfo->suffixesByLength[length] = table;
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        HTableString* table = NULL;
        size_t startIndex = startIndexByLengthNoHyphens[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_string_create(masterPartsNoHyphensCount);
            }
            for (size_t i = startIndex; i < masterPartsNoHyphensCount; i++) {
                MasterPart mp = masterPartsNoHyphens[i];
                const char* suffix = mp.partNumberNoHyphens + (mp.partNumberNoHyphensLength - length);
                htable_string_insert_if_not_exists(table, suffix, length, mp.partNumber);
            }
        }
        mpInfo->suffixesByNoHyphensLength[length] = table;
    }

    return mpInfo;
}

static PartsInfo* build_partsInfo(const Part* inputArray, size_t inputSize, size_t minLength) {
    Part* parts = malloc(inputSize * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t partsCount = 0;
    for (size_t i = 0; i < inputSize; i++) {
        const char* src = inputArray[i].partNumber;
        char buffer[MAX_STRING_LENGTH];
        size_t bufferLength;
        to_upper_trim(src, buffer, sizeof(buffer), &bufferLength);

        if (bufferLength >= minLength) {
            parts[partsCount].partNumberLength = bufferLength;
            parts[partsCount].partNumber = strdup(buffer);
            CHECK_ALLOC(parts[partsCount].partNumber);
            partsCount++;
        }
    }
    qsort(parts, partsCount, sizeof(*parts), compare_part_by_partNumber_length_asc);

    PartsInfo* partsInfo = malloc(sizeof(*partsInfo));
    CHECK_ALLOC(partsInfo);
    partsInfo->parts = parts;
    partsInfo->partsCount = partsCount;

    // Create and populate start indices.
    size_t startIndexByLength[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLength[length] = MAX_VALUE;
    }
    for (size_t i = 0; i < partsCount; i++) {
        size_t length = parts[i].partNumberLength;
        if (startIndexByLength[length] == MAX_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    backward_fill(startIndexByLength);

    // Create hash tables
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        partsInfo->suffixesByLength[length] = NULL;
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        HTableSizeList* table = NULL;
        size_t startIndex = startIndexByLength[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_sizelist_create(partsCount);
            }
            for (size_t i = startIndex; i < partsCount; i++) {
                Part part = parts[i];
                const char* suffix = part.partNumber + (part.partNumberLength - length);
                htable_sizelist_add(table, suffix, length, i);
            }
        }
        partsInfo->suffixesByLength[length] = table;
    }

    return partsInfo;
}

void processor_initialize(const SourceData* data) {
    masterPartsInfo = build_masterPartsInfo(data->masterParts, data->masterPartsCount);
    partsInfo = build_partsInfo(data->parts, data->partsCount, MIN_STRING_LENGTH);

    dictionary = htable_string_create(partsInfo->partsCount);

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

    for (long i = (long)masterPartsInfo->masterPartsCount - 1; i >= 0; i--) {
        MasterPart mp = masterPartsInfo->masterParts[i];
        HTableSizeList* partsBySuffix = partsInfo->suffixesByLength[mp.partNumberLength];
        if (partsBySuffix) {
            const SizeList* originalParts = htable_sizelist_search(partsBySuffix, mp.partNumber, mp.partNumberLength);
            if (originalParts) {
                for (long j = (long)originalParts->count - 1; j >= 0; j--) {
                    size_t originalPartIndex = originalParts->values[j];
                    Part part = partsInfo->parts[originalPartIndex];
                    htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, mp.partNumber);
                }
            }
        }
    }
}

const char* processor_find_match(const char* partNumber) {

    char buffer[MAX_STRING_LENGTH];
    size_t bufferLength;
    to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < MIN_STRING_LENGTH) {
        return NULL;
    }

    const char* match = htable_string_search(dictionary, buffer, bufferLength);
    return match;
}

void processor_clean() {
    free(masterPartsInfo->masterParts);
    free(masterPartsInfo->masterPartsNoHyphens);
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (masterPartsInfo->suffixesByLength[length]) {
            htable_string_free(masterPartsInfo->suffixesByLength[length]);
        }
        if (masterPartsInfo->suffixesByNoHyphensLength[length]) {
            htable_string_free(masterPartsInfo->suffixesByNoHyphensLength[length]);
        }
    }
    free(masterPartsInfo);
    for (size_t i = 0; i < partsInfo->partsCount; i++) {
        char* partNumber = (char*)partsInfo->parts[i].partNumber;
        free(partNumber);
    }
    free(partsInfo->parts);
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (partsInfo->suffixesByLength[length]) {
            htable_sizelist_free(partsInfo->suffixesByLength[length]);
        }
    }
    free(partsInfo);
    htable_string_free(dictionary);
}
