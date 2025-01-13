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
    HTableSizeList* suffixesByLength[MAX_LINE_LEN + 1];
} PartsInfo;

typedef struct MasterPartsInfo {
    MasterPart* masterParts;
    MasterPart* masterPartsNoHyphens;
    size_t masterPartsCount;
    size_t masterPartsNoHyphensCount;
    HTableString* suffixesByLength[MAX_LINE_LEN + 1];
    HTableString* suffixesByNoHyphensLength[MAX_LINE_LEN + 1];
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

static bool containsDash(const char* str, int strLength) {
    for (int i = 0; i < strLength; i++) {
        if (str[i] == '-') {
            return true;
        }
    }
    return false;
}

static MasterPart* build_masterPartsNyHyphen(MasterPart* masterParts, size_t masterPartsCount, size_t* outCount) {
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

static MasterPartsInfo* build_masterPartsInfo(MasterPart* masterParts, size_t masterPartsCount) {
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
    for (size_t length = 0; length <= MAX_LINE_LEN; length++) {
        mpInfo->suffixesByLength[length] = NULL;
        mpInfo->suffixesByNoHyphensLength[length] = NULL;
    }
    for (size_t length = 3; length <= MAX_LINE_LEN; length++) {
        HTableString* table = NULL;
        size_t startIndex = startIndexByLength[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_string_create(masterPartsCount);
            }
            for (size_t i = startIndex; i < masterPartsCount; i++) {
                MasterPart mp = masterParts[i];
                char* suffix = mp.partNumber + (mp.partNumberLength - length);
                htable_string_insert_if_not_exists(table, suffix, (int)length, mp.partNumber);
            }
        }
        mpInfo->suffixesByLength[length] = table;
    }
    for (size_t length = 3; length <= MAX_LINE_LEN; length++) {
        HTableString* table = NULL;
        size_t startIndex = startIndexByLengthNoHyphens[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_string_create(masterPartsNoHyphensCount);
            }
            for (size_t i = startIndex; i < masterPartsNoHyphensCount; i++) {
                MasterPart mp = masterPartsNoHyphens[i];
                char* suffix = mp.partNumberNoHyphens + (mp.partNumberNoHyphensLength - length);
                htable_string_insert_if_not_exists(table, suffix, (int)length, mp.partNumber);
            }
        }
        mpInfo->suffixesByNoHyphensLength[length] = table;
    }

    return mpInfo;
}

static PartsInfo* build_partsInfo(Part* inputArray, size_t inputSize, size_t minLen) {
    Part* parts = malloc(inputSize * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t partsCount = 0;
    for (size_t i = 0; i < inputSize; i++) {
        char* src = inputArray[i].partNumber;
        char buffer[MAX_LINE_LEN];
        size_t bufferLength;
        to_upper_trim(src, buffer, sizeof(buffer), &bufferLength);

        if (bufferLength >= minLen) {
            Part* part = &parts[partsCount];
            part->partNumberLength = (int)bufferLength;
            part->partNumber = malloc(bufferLength + 1);
            CHECK_ALLOC(part->partNumber);
            strcpy(part->partNumber, buffer);
            partsCount++;
        }
    }
    qsort(parts, partsCount, sizeof(*parts), compare_part_by_partNumber_length_asc);

    PartsInfo* partsInfo = malloc(sizeof(*partsInfo));
    CHECK_ALLOC(partsInfo);
    partsInfo->parts = parts;
    partsInfo->partsCount = partsCount;

    // Create and populate start indices.
    size_t startIndexByLength[MAX_LINE_LEN + 1] = { 0 };
    for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
        startIndexByLength[i] = MAX_VALUE;
    }
    for (size_t i = 0; i < partsCount; i++) {
        size_t length = parts[i].partNumberLength;
        if (startIndexByLength[length] == MAX_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    backward_fill(startIndexByLength);

    // Create hash tables
    for (size_t length = 0; length <= MAX_LINE_LEN; length++) {
        partsInfo->suffixesByLength[length] = NULL;
    }
    for (size_t length = 3; length <= MAX_LINE_LEN; length++) {
        HTableSizeList* table = NULL;
        size_t startIndex = startIndexByLength[length];
        if (startIndex != MAX_VALUE) {
            if (!table) {
                table = htable_sizelist_create(partsCount);
            }
            for (size_t i = startIndex; i < partsCount; i++) {
                Part part = parts[i];
                char* suffix = part.partNumber + (part.partNumberLength - length);
                htable_sizelist_add(table, suffix, (int)length, i);
            }
        }
        partsInfo->suffixesByLength[length] = table;
    }

    return partsInfo;
}

void processor_initialize(SourceData* data) {
    masterPartsInfo = build_masterPartsInfo(data->masterParts, data->masterPartsCount);
    partsInfo = build_partsInfo(data->parts, data->partsCount, 3);

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

    for (int i = (int)masterPartsInfo->masterPartsCount - 1; i >= 0; i--) {
        MasterPart mp = masterPartsInfo->masterParts[i];
        HTableSizeList* partsBySuffix = partsInfo->suffixesByLength[mp.partNumberLength];
        if (partsBySuffix) {
            const SizeList* originalParts = htable_sizelist_search(partsBySuffix, mp.partNumber, mp.partNumberLength);
            if (originalParts) {
                for (int j = (int)originalParts->count - 1; j >= 0; j--) {
                    size_t originalPartIndex = originalParts->values[j];
                    Part part = partsInfo->parts[originalPartIndex];
                    htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, mp.partNumber);
                }
            }
        }
    }
}

const char* processor_find_match(char* partNumber) {

    char buffer[MAX_LINE_LEN];
    size_t bufferLength;
    to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < 3) {
        return NULL;
    }

    const char* match = htable_string_search(dictionary, buffer, (int)bufferLength);
    return match;
}

void processor_clean() {
    free(masterPartsInfo->masterPartsNoHyphens);
    for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
        if (masterPartsInfo->suffixesByLength[i]) {
            htable_string_free(masterPartsInfo->suffixesByLength[i]);
        }
        if (masterPartsInfo->suffixesByNoHyphensLength[i]) {
            htable_string_free(masterPartsInfo->suffixesByNoHyphensLength[i]);
        }
    }
    free(masterPartsInfo);
    free(partsInfo->parts);
    for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
        if (partsInfo->suffixesByLength[i]) {
            htable_sizelist_free(partsInfo->suffixesByLength[i]);
        }
    }
    free(partsInfo);
    htable_string_free(dictionary);
}
