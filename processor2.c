#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "source_data.h"
#include "processor.h"

const char *processor_get_identifier() { return "Processor2"; }

static int compare_mp_by_partNumber_length_asc(const void *a, const void *b);
static int compare_mp_by_partNumberNoHyphens_length_asc(const void *a, const void *b);
static void backward_fill(size_t *array);
static void forward_fill(size_t *array);

static const size_t MAX_VALUE = ((size_t)-1);
static MasterPart *masterPartsAsc = NULL;
static MasterPart *masterPartsAscByNoHyphens = NULL;
static size_t masterPartsCount = 0;
static size_t startIndexByLengthAsc[MAX_STRING_LENGTH];
static size_t startIndexByLengthAscNoHyphens[MAX_STRING_LENGTH];
static size_t startIndexByLengthDesc[MAX_STRING_LENGTH];

const char *processor_find_match(const char *partNumber) {

    char buffer[MAX_STRING_LENGTH];
    size_t bufferLength;
    str_to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < MIN_STRING_LENGTH) {
        return NULL;
    }

    size_t startIndex = startIndexByLengthAsc[bufferLength];
    if (startIndex != MAX_VALUE) {
        for (size_t i = startIndex; i < masterPartsCount; i++) {
            MasterPart mp = masterPartsAsc[i];
            if (str_is_suffix_vectorized(buffer, bufferLength, mp.partNumber, mp.partNumberLength)) {
                return mp.partNumber;
            }
        }
    }

    startIndex = startIndexByLengthAscNoHyphens[bufferLength];
    if (startIndex != MAX_VALUE) {
        for (size_t i = startIndex; i < masterPartsCount; i++) {
            MasterPart mp = masterPartsAscByNoHyphens[i];
            if (str_is_suffix_vectorized(buffer, bufferLength, mp.partNumberNoHyphens, mp.partNumberNoHyphensLength)) {
                return mp.partNumber;
            }
        }
    }

    startIndex = startIndexByLengthDesc[bufferLength];
    if (startIndex != MAX_VALUE) {
        for (long i = (long)startIndex; i >= 0; i--) {
            MasterPart mp = masterPartsAsc[i];
            if (str_is_suffix_vectorized(mp.partNumber, mp.partNumberLength, buffer, bufferLength)) {
                return mp.partNumber;
            }
        }
    }

    return NULL;
}

void processor_initialize(const SourceData *data) {
    masterPartsCount = data->masterPartsCount;
    masterPartsAsc = (MasterPart *)malloc(masterPartsCount * sizeof(*masterPartsAsc));
    CHECK_ALLOC(masterPartsAsc);
    memcpy(masterPartsAsc, data->masterParts, masterPartsCount * sizeof(*masterPartsAsc));
    qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_mp_by_partNumber_length_asc);

    masterPartsAscByNoHyphens = (MasterPart *)malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    CHECK_ALLOC(masterPartsAscByNoHyphens);
    memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLengthAsc[length] = MAX_VALUE;
        startIndexByLengthAscNoHyphens[length] = MAX_VALUE;
        startIndexByLengthDesc[length] = MAX_VALUE;
    }

    // Populate the start indices
    for (size_t i = 0; i < masterPartsCount; i++) {
        size_t length = masterPartsAsc[i].partNumberLength;
        if (startIndexByLengthAsc[length] == MAX_VALUE) {
            startIndexByLengthAsc[length] = i;
        }
        startIndexByLengthDesc[length] = i;

        length = masterPartsAscByNoHyphens[i].partNumberNoHyphensLength;
        if (startIndexByLengthAscNoHyphens[length] == MAX_VALUE) {
            startIndexByLengthAscNoHyphens[length] = i;
        }
    }

    backward_fill(startIndexByLengthAsc);
    backward_fill(startIndexByLengthAscNoHyphens);
    forward_fill(startIndexByLengthDesc);
}

void processor_clean() {
    if (masterPartsAsc) {
        free(masterPartsAsc);
        masterPartsAsc = NULL;
    }
    if (masterPartsAscByNoHyphens) {
        free(masterPartsAscByNoHyphens);
        masterPartsAscByNoHyphens = NULL;
    }
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

static void forward_fill(size_t *array) {
    size_t tmp = array[0];
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
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
