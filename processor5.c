#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "thread_utils.h"
#include "hash_table.h"
#include "cross_platform_time.h"
#include "processor.h"

const char *processor_get_identifier() { return "Processor5"; }

typedef struct PartsInfo {
    Part *parts;
    size_t partsCount;
    HTableSizeList *suffixesByLength[MAX_STRING_LENGTH];
} PartsInfo;

typedef struct MasterPartsInfo {
    MasterPart *masterParts;
    MasterPart *masterPartsNoHyphens;
    size_t masterPartsCount;
    size_t masterPartsNoHyphensCount;
    HTableString *suffixesByLength[MAX_STRING_LENGTH];
    HTableString *suffixesByNoHyphensLength[MAX_STRING_LENGTH];
} MasterPartsInfo;

typedef struct ThreadArgs {
    void *info;
    size_t startIndex;
    size_t suffixLength;
} ThreadArgs;

static void build_masterPartsInfo(const SourceData *data, MasterPartsInfo *mpInfo);
static void build_partsInfo(const SourceData *data, PartsInfo *partsInfo);
static thread_ret_t create_suffix_table_for_mp_PartNumber(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_mp_PartNumberNoHyphens(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_part_PartNumber(thread_arg_t arg);
static void free_info_allocations(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo);
static int compare_mp_by_partNumber_length_asc(const void *a, const void *b);
static int compare_mp_by_partNumberNoHyphens_length_asc(const void *a, const void *b);
static int compare_part_by_partNumber_length_asc(const void *a, const void *b);
static void backward_fill(size_t *array);

static const size_t MAX_VALUE = ((size_t)-1);
static char *block = NULL;
static HTableString *dictionary = NULL;

const char *processor_find_match(const char *partNumber) {

    char buffer[MAX_STRING_LENGTH];
    size_t bufferLength;
    str_to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < MIN_STRING_LENGTH) {
        return NULL;
    }

    const char *match = htable_string_search(dictionary, buffer, bufferLength);
    return match;
}

void processor_initialize(const SourceData *data) {
    MasterPartsInfo masterPartsInfo = { 0 };
    build_masterPartsInfo(data, &masterPartsInfo);
    PartsInfo partsInfo = { 0 };
    build_partsInfo(data, &partsInfo);

    dictionary = htable_string_create(partsInfo.partsCount);

    for (size_t i = 0; i < partsInfo.partsCount; i++) {
        Part part = partsInfo.parts[i];

        HTableString *masterPartsBySuffix = masterPartsInfo.suffixesByLength[part.partNumberLength];
        if (masterPartsBySuffix) {
            const char *match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
                continue;
            }
        }
        masterPartsBySuffix = masterPartsInfo.suffixesByNoHyphensLength[part.partNumberLength];
        if (masterPartsBySuffix) {
            const char *match = htable_string_search(masterPartsBySuffix, part.partNumber, part.partNumberLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.partNumber, part.partNumberLength, match);
            }
        }
    }

    for (long i = (long)masterPartsInfo.masterPartsCount - 1; i >= 0; i--) {
        MasterPart mp = masterPartsInfo.masterParts[i];
        HTableSizeList *partsBySuffix = partsInfo.suffixesByLength[mp.partNumberLength];
        if (partsBySuffix) {
            const ListItem *originalParts = htable_sizelist_search(partsBySuffix, mp.partNumber, mp.partNumberLength);
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

static void build_masterPartsInfo(const SourceData *data, MasterPartsInfo *mpInfo) {
    // Build masterParts
    size_t masterPartsCount = data->masterPartsCount;
    MasterPart *masterParts = malloc(masterPartsCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);
    memcpy(masterParts, data->masterParts, masterPartsCount * sizeof(*masterParts));
    qsort(masterParts, masterPartsCount, sizeof(*masterParts), compare_mp_by_partNumber_length_asc);

    // Build masterPartsNoHyphens
    MasterPart *masterPartsNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsNoHyphens));
    CHECK_ALLOC(masterPartsNoHyphens);
    size_t masterPartsNoHyphensCount = 0;
    for (size_t i = 0; i < masterPartsCount; i++) {
        if (str_contains_dash(masterParts[i].partNumber, masterParts[i].partNumberLength)) {
            masterPartsNoHyphens[masterPartsNoHyphensCount++] = masterParts[i];
        }
    }
    qsort(masterPartsNoHyphens, masterPartsNoHyphensCount, sizeof(*masterPartsNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    // Populate MasterPartsInfo
    mpInfo->masterParts = masterParts;
    mpInfo->masterPartsCount = masterPartsCount;
    mpInfo->masterPartsNoHyphens = masterPartsNoHyphens;
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

    // Create a thread for each suffixesByLength table
    thread_t threads[MAX_STRING_LENGTH] = { 0 };
    ThreadArgs threadArgs[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLength[length] != MAX_VALUE) {
            threadArgs[length].info = mpInfo;
            threadArgs[length].suffixLength = length;
            threadArgs[length].startIndex = startIndexByLength[length];
            int status = create_thread(&threads[length], create_suffix_table_for_mp_PartNumber, &threadArgs[length]);
            CHECK_THREAD_CREATE_STATUS(status, length);
        }
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (threads[length]) {
            int status = join_thread(threads[length], NULL);
            CHECK_THREAD_JOIN_STATUS(status, length);
        }
    }

    // Create a thread for each suffixesByNoHyphensLength table
    thread_t threads2[MAX_STRING_LENGTH] = { 0 };
    ThreadArgs threadArgs2[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLengthNoHyphens[length] != MAX_VALUE) {
            threadArgs2[length].info = mpInfo;
            threadArgs2[length].suffixLength = length;
            threadArgs2[length].startIndex = startIndexByLengthNoHyphens[length];
            int status = create_thread(&threads2[length], create_suffix_table_for_mp_PartNumberNoHyphens, &threadArgs2[length]);
            CHECK_THREAD_CREATE_STATUS(status, length);
        }
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (threads2[length]) {
            int status = join_thread(threads2[length], NULL);
            CHECK_THREAD_JOIN_STATUS(status, length);
        }
    }
}

static thread_ret_t create_suffix_table_for_mp_PartNumber(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    MasterPartsInfo *mpInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    HTableString *table = NULL;
    MasterPart *masterParts = mpInfo->masterParts;
    size_t masterPartsCount = mpInfo->masterPartsCount;

    table = htable_string_create(masterPartsCount - startIndex);
    for (size_t i = startIndex; i < masterPartsCount; i++) {
        MasterPart mp = masterParts[i];
        const char *suffix = mp.partNumber + (mp.partNumberLength - suffixLength);
        htable_string_insert_if_not_exists(table, suffix, suffixLength, mp.partNumber);
    }
    mpInfo->suffixesByLength[suffixLength] = table;
    return 0;
}

static thread_ret_t create_suffix_table_for_mp_PartNumberNoHyphens(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    MasterPartsInfo *mpInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    HTableString *table = NULL;
    MasterPart *masterPartsNoHyphens = mpInfo->masterPartsNoHyphens;
    size_t masterPartsNoHyphensCount = mpInfo->masterPartsNoHyphensCount;

    table = htable_string_create(masterPartsNoHyphensCount - startIndex);
    for (size_t i = startIndex; i < masterPartsNoHyphensCount; i++) {
        MasterPart mp = masterPartsNoHyphens[i];
        const char *suffix = mp.partNumberNoHyphens + (mp.partNumberNoHyphensLength - suffixLength);
        htable_string_insert_if_not_exists(table, suffix, suffixLength, mp.partNumber);
    }
    mpInfo->suffixesByNoHyphensLength[suffixLength] = table;
    return 0;
}

static void build_partsInfo(const SourceData *data, PartsInfo *partsInfo) {
    const Part *inputParts = data->parts;
    size_t inputPartsCount = data->partsCount;

    // Allocate block memory for strings
    size_t blockIndex = 0;
    size_t blockSize = sizeof(char) * MAX_STRING_LENGTH * inputPartsCount;
    block = malloc(blockSize);
    CHECK_ALLOC(block);

    // Build parts
    Part *parts = malloc(sizeof(*parts) * inputPartsCount);
    CHECK_ALLOC(parts);
    size_t partsCount = 0;
    for (size_t i = 0; i < inputPartsCount; i++) {
        const char *src = inputParts[i].partNumber;
        size_t stringLength;
        str_to_upper_trim(src, &block[blockIndex], MAX_STRING_LENGTH, &stringLength);

        if (stringLength >= MIN_STRING_LENGTH) {
            parts[partsCount].partNumberLength = stringLength;
            parts[partsCount].partNumber = &block[blockIndex];
            CHECK_ALLOC(parts[partsCount].partNumber);
            partsCount++;
        }
        blockIndex += stringLength + 1;
    }
    qsort(parts, partsCount, sizeof(*parts), compare_part_by_partNumber_length_asc);

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

    // Create a thread for each suffixesByLength table
    thread_t threads[MAX_STRING_LENGTH] = { 0 };
    ThreadArgs threadArgs[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLength[length] != MAX_VALUE) {
            threadArgs[length].info = partsInfo;
            threadArgs[length].suffixLength = length;
            threadArgs[length].startIndex = startIndexByLength[length];
            int status = create_thread(&threads[length], create_suffix_table_for_part_PartNumber, &threadArgs[length]);
            CHECK_THREAD_CREATE_STATUS(status, length);
        }
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (threads[length]) {

            int status = join_thread(threads[length], NULL);
            CHECK_THREAD_JOIN_STATUS(status, length);
        }
    }
}

static thread_ret_t create_suffix_table_for_part_PartNumber(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    PartsInfo *partsInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    HTableSizeList *table = NULL;
    Part *parts = partsInfo->parts;
    size_t partsCount = partsInfo->partsCount;

    table = htable_sizelist_create(partsCount - startIndex);
    for (size_t i = startIndex; i < partsCount; i++) {
        Part part = parts[i];
        const char *suffix = part.partNumber + (part.partNumberLength - suffixLength);
        htable_sizelist_add(table, suffix, suffixLength, i);
    }
    partsInfo->suffixesByLength[suffixLength] = table;
    return 0;
}

static void backward_fill(size_t *array) {
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

static int compare_mp_by_partNumber_length_asc(const void *a, const void *b) {
    size_t lenA = ((const MasterPart *)a)->partNumberLength;
    size_t lenB = ((const MasterPart *)b)->partNumberLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static int compare_mp_by_partNumberNoHyphens_length_asc(const void *a, const void *b) {
    size_t lenA = ((const MasterPart *)a)->partNumberNoHyphensLength;
    size_t lenB = ((const MasterPart *)b)->partNumberNoHyphensLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static int compare_part_by_partNumber_length_asc(const void *a, const void *b) {
    size_t lenA = ((const Part *)a)->partNumberLength;
    size_t lenB = ((const Part *)b)->partNumberLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}
