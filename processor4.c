#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"
#include "cross_platform_time.h"
#include "processor.h"

const char* processor_get_identifier() { return "Processor4"; }

static int compare_mp_by_partNumber_length_asc(const void* a, const void* b);
static int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b);
static int compare_part_by_partNumber_length_asc(const void* a, const void* b);
static void backward_fill(size_t* array);

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

static const size_t MAX_VALUE = ((size_t)-1);
static char* block = NULL;
static HTableString* dictionary = NULL;

const char* processor_find_match(const char* partNumber) {

    char buffer[MAX_STRING_LENGTH];
    size_t bufferLength;
    str_to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < MIN_STRING_LENGTH) {
        return NULL;
    }

    const char* match = htable_string_search(dictionary, buffer, bufferLength);
    return match;
}

static MasterPartsInfo build_masterPartsInfo(const MasterPart* inputArray, size_t inputArrayCount);
static PartsInfo build_partsInfo(const Part* inputArray, size_t inputSize, size_t minLength);
static void free_info_allocations(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo);

void processor_initialize(const SourceData* data) {
    MasterPartsInfo masterPartsInfo = build_masterPartsInfo(data->masterParts, data->masterPartsCount);
    PartsInfo partsInfo = build_partsInfo(data->parts, data->partsCount, MIN_STRING_LENGTH);

    dictionary = htable_string_create(partsInfo.partsCount);

    for (size_t i = 0; i < partsInfo.partsCount; i++) {
        Part part = partsInfo.parts[i];

        HTableString* masterPartsBySuffix = masterPartsInfo.suffixesByLength[part.partNumberLength];
        if (masterPartsBySuffix) {
            const char* match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
            }
        }
        masterPartsBySuffix = masterPartsInfo.suffixesByNoHyphensLength[part.partNumberLength];
        if (masterPartsBySuffix) {
            const char* match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
            }
        }
    }

    for (long i = (long)masterPartsInfo.masterPartsCount - 1; i >= 0; i--) {
        MasterPart mp = masterPartsInfo.masterParts[i];
        HTableSizeList* partsBySuffix = partsInfo.suffixesByLength[mp.partNumberLength];
        if (partsBySuffix) {
            const ListItem* originalParts = htable_sizelist_search(partsBySuffix, mp.partNumber, mp.partNumberLength);
            while (originalParts) {
                size_t originalPartIndex = originalParts->value;
                Part part = partsInfo.parts[originalPartIndex];
                htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, mp.partNumber);
                originalParts = originalParts->next;
            }
        }
    }

    free_info_allocations(masterPartsInfo, partsInfo);
}

static void free_info_allocations(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo) {
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (masterPartsInfo.suffixesByLength[length]) {
            htable_string_free(masterPartsInfo.suffixesByLength[length]);
        }
        if (masterPartsInfo.suffixesByNoHyphensLength[length]) {
            htable_string_free(masterPartsInfo.suffixesByNoHyphensLength[length]);
        }
    }
    free(masterPartsInfo.masterParts);
    free(masterPartsInfo.masterPartsNoHyphens);

    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (partsInfo.suffixesByLength[length]) {
            htable_sizelist_free(partsInfo.suffixesByLength[length]);
        }
    }
    free(partsInfo.parts);
}

void processor_clean() {
    free(block);
    htable_string_free(dictionary);
}

static MasterPartsInfo build_masterPartsInfo(const MasterPart* inputArray, size_t inputArrayCount) {
    // Build masterParts
    size_t masterPartsCount = inputArrayCount;
    MasterPart* masterParts = malloc(masterPartsCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);
    memcpy(masterParts, inputArray, masterPartsCount * sizeof(*masterParts));
    qsort(masterParts, masterPartsCount, sizeof(*masterParts), compare_mp_by_partNumber_length_asc);

    // Build masterPartsNoHyphens
    MasterPart* masterPartsNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsNoHyphens));
    CHECK_ALLOC(masterPartsNoHyphens);
    size_t masterPartsNoHyphensCount = 0;
    for (size_t i = 0; i < masterPartsCount; i++) {
        if (str_contains_dash(masterParts[i].partNumber, masterParts[i].partNumberLength)) {
            masterPartsNoHyphens[masterPartsNoHyphensCount++] = masterParts[i];
        }
    }
    qsort(masterPartsNoHyphens, masterPartsNoHyphensCount, sizeof(*masterPartsNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    // Initialize MasterPartsInfo
    MasterPartsInfo mpInfo = {
        .masterParts = masterParts,
        .masterPartsNoHyphens = masterPartsNoHyphens,
        .masterPartsCount = masterPartsCount,
        .masterPartsNoHyphensCount = masterPartsNoHyphensCount,
        .suffixesByLength = { NULL },
        .suffixesByNoHyphensLength = { NULL }
    };

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
        mpInfo.suffixesByLength[length] = NULL;
        mpInfo.suffixesByNoHyphensLength[length] = NULL;
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
        mpInfo.suffixesByLength[length] = table;
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
        mpInfo.suffixesByNoHyphensLength[length] = table;
    }

    return mpInfo;
}

static PartsInfo build_partsInfo(const Part* inputArray, size_t inputSize, size_t minLength) {
    // Allocate block memory for strings
    size_t blockIndex = 0;
    size_t blockSize = sizeof(char) * MAX_STRING_LENGTH * inputSize;
    block = malloc(blockSize);
    CHECK_ALLOC(block);

    // Build parts
    Part* parts = malloc(sizeof(*parts) * inputSize);
    CHECK_ALLOC(parts);
    size_t partsCount = 0;
    for (size_t i = 0; i < inputSize; i++) {
        const char* src = inputArray[i].partNumber;
        size_t stringLength;
        str_to_upper_trim(src, &block[blockIndex], MAX_STRING_LENGTH, &stringLength);

        if (stringLength >= minLength) {
            parts[partsCount].partNumberLength = stringLength;
            parts[partsCount].partNumber = &block[blockIndex];
            CHECK_ALLOC(parts[partsCount].partNumber);
            partsCount++;
        }
        blockIndex += stringLength + 1;
    }
    qsort(parts, partsCount, sizeof(*parts), compare_part_by_partNumber_length_asc);

    // Initialize PartsInfo
    PartsInfo partsInfo = {
        .parts = parts,
        .partsCount = partsCount,
        .suffixesByLength = { NULL }
    };

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
        partsInfo.suffixesByLength[length] = NULL;
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
        partsInfo.suffixesByLength[length] = table;
    }

    return partsInfo;
}

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

static int compare_mp_by_partNumber_length_asc(const void* a, const void* b) {
    size_t lenA = ((const MasterPart*)a)->partNumberLength;
    size_t lenB = ((const MasterPart*)b)->partNumberLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b) {
    size_t lenA = ((const MasterPart*)a)->partNumberNoHyphensLength;
    size_t lenB = ((const MasterPart*)b)->partNumberNoHyphensLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static int compare_part_by_partNumber_length_asc(const void* a, const void* b) {
    size_t lenA = ((const Part*)a)->partNumberLength;
    size_t lenB = ((const Part*)b)->partNumberLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}
