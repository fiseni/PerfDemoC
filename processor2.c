#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "source_data.h"
#include "processor.h"

const size_t MAX_VALUE = ((size_t)-1);

const char* processor_get_identifier() {
    return "Processor2";
}

MasterPart* masterPartsAsc = NULL;
MasterPart* masterPartsAscByNoHyphens = NULL;
size_t masterPartsCount = 0;
size_t startIndexByLengthAsc[MAX_LINE_LEN + 1];
size_t startIndexByLengthAscNoHyphens[MAX_LINE_LEN + 1];
size_t startIndexByLengthDesc[MAX_LINE_LEN + 1];

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

void processor_initialize(SourceData* data) {
    masterPartsAsc = data->masterParts;
    masterPartsCount = data->masterPartsCount;
    qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_mp_by_partNumber_length_asc);

    masterPartsAscByNoHyphens = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    assert(masterPartsAscByNoHyphens);
    memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    for (size_t i = 0; i <= MAX_LINE_LEN; i++) {
        startIndexByLengthAsc[i] = MAX_VALUE;
        startIndexByLengthAscNoHyphens[i] = MAX_VALUE;
        startIndexByLengthDesc[i] = MAX_VALUE;
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

    size_t startIndex = startIndexByLengthAsc[bufferLen];
    if (startIndex != MAX_VALUE) {
        for (size_t i = startIndex; i < masterPartsCount; i++) {
            MasterPart mp = masterPartsAsc[i];
            if (is_suffix_vectorized(buffer, bufferLen, mp.partNumber, mp.partNumberLength)) {
                return mp.partNumber;
            }
        }
    }

    startIndex = startIndexByLengthAscNoHyphens[bufferLen];
    if (startIndex != MAX_VALUE) {
        for (size_t i = startIndex; i < masterPartsCount; i++) {
            MasterPart mp = masterPartsAscByNoHyphens[i];
            if (is_suffix_vectorized(buffer, bufferLen, mp.partNumberNoHyphens, mp.partNumberNoHyphensLength)) {
                return mp.partNumber;
            }
        }
    }

    startIndex = startIndexByLengthDesc[bufferLen];
    if (startIndex != MAX_VALUE) {
        for (long i = (long)startIndex; i >= 0; i--) {
            MasterPart mp = masterPartsAsc[i];
            if (is_suffix_vectorized(mp.partNumber, mp.partNumberLength, buffer, bufferLen)) {
                return mp.partNumber;
            }
        }
    }

    return NULL;
}

void processor_clean() {
    if (masterPartsAscByNoHyphens) {
        free(masterPartsAscByNoHyphens);
        masterPartsAscByNoHyphens = NULL;
    }
}
