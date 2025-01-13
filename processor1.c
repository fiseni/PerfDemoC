#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "source_data.h"
#include "processor.h"

const char* processor_get_identifier() {
    return "Processor1";
}

MasterPart* masterPartsAsc = NULL;
MasterPart* masterPartsAscByNoHyphens = NULL;
MasterPart* masterPartsDesc = NULL;
size_t masterPartsCount = 0;

void processor_initialize(SourceData* data) {
    masterPartsCount = data->masterPartsCount;
    masterPartsAsc = data->masterParts;
    qsort(masterPartsAsc, masterPartsCount, sizeof(*masterPartsAsc), compare_mp_by_partNumber_length_asc);

    masterPartsAscByNoHyphens = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    assert(masterPartsAscByNoHyphens);
    memcpy(masterPartsAscByNoHyphens, masterPartsAsc, masterPartsCount * sizeof(*masterPartsAscByNoHyphens));
    qsort(masterPartsAscByNoHyphens, masterPartsCount, sizeof(*masterPartsAscByNoHyphens), compare_mp_by_partNumberNoHyphens_length_asc);

    masterPartsDesc = (MasterPart*)malloc(masterPartsCount * sizeof(*masterPartsDesc));
    assert(masterPartsDesc);
    memcpy(masterPartsDesc, masterPartsAsc, masterPartsCount * sizeof(*masterPartsDesc));
    qsort(masterPartsDesc, masterPartsCount, sizeof(*masterPartsDesc), compare_mp_by_partNumber_length_desc);
}

const char* processor_find_match(char* partNumber) {

    char buffer[MAX_LINE_LEN];
    size_t bufferLength;
    to_upper_trim(partNumber, buffer, sizeof(buffer), &bufferLength);
    if (bufferLength < 3) {
        return NULL;
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsAsc[i];
        if (is_suffix(buffer, bufferLength, mp.partNumber, mp.partNumberLength)) {
            return mp.partNumber;
        }
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsAscByNoHyphens[i];
        if (is_suffix(buffer, bufferLength, mp.partNumberNoHyphens, mp.partNumberNoHyphensLength)) {
            return mp.partNumber;
        }
    }

    for (size_t i = 0; i < masterPartsCount; i++) {
        MasterPart mp = masterPartsDesc[i];
        if (is_suffix(mp.partNumber, mp.partNumberLength, buffer, bufferLength)) {
            return mp.partNumber;
        }
    }

    return NULL;
}

void processor_clean() {
    if (masterPartsAscByNoHyphens) {
        free(masterPartsAscByNoHyphens);
        masterPartsAscByNoHyphens = NULL;
    }

    if (masterPartsDesc) {
        free(masterPartsDesc);
        masterPartsDesc = NULL;
    }
}
