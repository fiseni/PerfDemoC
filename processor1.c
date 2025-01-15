#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "source_data.h"
#include "processor.h"

const char* processor_get_identifier() { return "Processor1"; }

static int compare_mp_by_partNumber_length_asc(const void* a, const void* b);
static int compare_mp_by_partNumber_length_desc(const void* a, const void* b);
static int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b);

static MasterPart* masterPartsAsc = NULL;
static MasterPart* masterPartsAscByNoHyphens = NULL;
static MasterPart* masterPartsDesc = NULL;
static size_t masterPartsCount = 0;

const char* processor_find_match(const char* partNumber) {

    char buffer[MAX_STRING_LENGTH];
    size_t bufferLength;
    str_to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < MIN_STRING_LENGTH) {
        return NULL;
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsAsc[i];
        if (str_is_suffix(buffer, bufferLength, mp.partNumber, mp.partNumberLength)) {
            return mp.partNumber;
        }
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsAscByNoHyphens[i];
        if (str_is_suffix(buffer, bufferLength, mp.partNumberNoHyphens, mp.partNumberNoHyphensLength)) {
            return mp.partNumber;
        }
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsDesc[i];
        if (str_is_suffix(mp.partNumber, mp.partNumberLength, buffer, bufferLength)) {
            return mp.partNumber;
        }
    }

    return NULL;
}

void processor_initialize(const SourceData* data) {
    masterPartsCount = data->masterPartsCount;
    masterPartsAsc = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsAsc));
    CHECK_ALLOC(masterPartsAsc);
    memcpy(masterPartsAsc, data->masterParts, masterPartsCount * sizeof(*masterPartsAsc));
    qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_mp_by_partNumber_length_asc);

    masterPartsAscByNoHyphens = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    CHECK_ALLOC(masterPartsAscByNoHyphens);
    memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    masterPartsDesc = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsDesc));
    CHECK_ALLOC(masterPartsDesc);
    memcpy(masterPartsDesc, masterPartsAsc, masterPartsCount * sizeof(*masterPartsDesc));
    qsort(masterPartsDesc, masterPartsCount, sizeof(*masterPartsDesc), compare_mp_by_partNumber_length_desc);
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
    if (masterPartsDesc) {
        free(masterPartsDesc);
        masterPartsDesc = NULL;
    }
}

static int compare_mp_by_partNumber_length_asc(const void* a, const void* b) {
    size_t lenA = ((const MasterPart*)a)->partNumberLength;
    size_t lenB = ((const MasterPart*)b)->partNumberLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}

static int compare_mp_by_partNumber_length_desc(const void* a, const void* b) {
    size_t lenA = ((const MasterPart*)a)->partNumberLength;
    size_t lenB = ((const MasterPart*)b)->partNumberLength;
    return lenA < lenB ? 1 : lenA > lenB ? -1 : 0;
}

static int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b) {
    size_t lenA = ((const MasterPart*)a)->partNumberNoHyphensLength;
    size_t lenB = ((const MasterPart*)b)->partNumberNoHyphensLength;
    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
}
