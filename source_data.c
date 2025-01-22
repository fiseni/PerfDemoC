#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "utils.h"
#include "cross_platform_time.h"
#include "source_data.h"

static Part *build_parts(const char *partsPath, size_t *outCount);
static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount);
static bool populate_masterPart(MasterPart *masterPart, char *partNumber, size_t partNumberLength, char *block, size_t blockSize, size_t *blockIndexNoHyphens);

const SourceData *source_data_read(const char *masterPartsPath, const char *partsPath) {
    size_t masterPartsCount = 0;
    MasterPart *masterParts = build_masterParts(masterPartsPath, &masterPartsCount);
    size_t partsCount = 0;
    Part *parts = build_parts(partsPath, &partsCount);

    SourceData *data = (SourceData *)malloc(sizeof(*data));
    CHECK_ALLOC(data);
    data->masterParts = masterParts;
    data->masterPartsCount = masterPartsCount;
    data->parts = parts;
    data->partsCount = partsCount;

    return data;
}

void source_data_clean(const SourceData *data) {
    // All strings are allocated from a single block
    free((void *)data->masterParts->partNumber);
    free((void *)data->parts->partNumber);

    free((void *)data->masterParts);
    free((void *)data->parts);
    free((void *)data);
}

static Part *build_parts(const char *partsPath, size_t *outCount) {
    long fileSize = get_file_size_bytes(partsPath);
    FILE *file = fopen(partsPath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open parts file: %s\n", partsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * fileSize + 1;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

    if (contentSize > 0 && block[contentSize - 1] != '\n') {
        block[contentSize] = '\n';
        contentSize++;
    }

    size_t lineCount = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            lineCount++;
        }
    }
    Part *parts = malloc(lineCount * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t stringStartIndex = 0;
    size_t partsIndex = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            size_t length = i - stringStartIndex;
            if (i > 0 && block[i - 1] == '\r') {
                block[i - 1] = '\0';
                length--;
            }
            assert(partsIndex < lineCount);
            parts[partsIndex].partNumber = &block[stringStartIndex];
            parts[partsIndex].partNumberLength = length;
            partsIndex++;
            stringStartIndex = i + 1;
        }
    }

    *outCount = partsIndex;
    return parts;
}

static MasterPart *build_masterParts(const char *masterPartsPath, size_t *outCount) {
    long fileSize = get_file_size_bytes(masterPartsPath);
    FILE *file = fopen(masterPartsPath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open file: %s\n", masterPartsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * fileSize * 2 + 2;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

    if (contentSize > 0 && block[contentSize - 1] != '\n') {
        block[contentSize] = '\n';
        contentSize++;
    }

    size_t lineCount = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            lineCount++;
        }
    }
    MasterPart *masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);

    size_t stringStartIndex = 0;
    size_t masterPartsIndex = 0;
    size_t blockIndexNoHyphens = contentSize;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            size_t length = i - stringStartIndex;
            if (i > 0 && block[i - 1] == '\r') {
                block[i - 1] = '\0';
                length--;
            }
            assert(masterPartsIndex < lineCount);
            if (populate_masterPart(&masterParts[masterPartsIndex], &block[stringStartIndex], length, block, blockSize, &blockIndexNoHyphens)) {
                masterPartsIndex++;
            }
            stringStartIndex = i + 1;
        }
    }

    *outCount = masterPartsIndex;
    return masterParts;
}

static bool populate_masterPart(MasterPart *masterPart, char *partNumber, size_t partNumberLength, char *block, size_t blockSize, size_t *blockIndexNoHyphens) {
    if (partNumberLength < MIN_STRING_LENGTH) {
        return false;
    }

    str_to_upper_trim_in_place(partNumber, partNumberLength, &partNumberLength);
    if (partNumberLength < MIN_STRING_LENGTH) {
        return false;
    }

    masterPart->partNumber = partNumber;
    masterPart->partNumberLength = partNumberLength;

    if (str_contains_dash(partNumber, partNumberLength)) {
        // Ensure there is enough space in the block for partNumberNoHyphens
        if (*blockIndexNoHyphens + partNumberLength + 1 > blockSize) { // +1 for null terminator
            fprintf(stderr, "Block overflow detected while allocating partNumberNoHyphens.\n");
            exit(EXIT_FAILURE);
        }

        char *partNumberNoHyphens = &block[*blockIndexNoHyphens];
        size_t partNumberNoHyphensLength;
        str_remove_char(partNumber, partNumberLength, partNumberNoHyphens, partNumberLength, '-', &partNumberNoHyphensLength);
        masterPart->partNumberNoHyphens = partNumberNoHyphens;
        masterPart->partNumberNoHyphensLength = partNumberNoHyphensLength;

        *blockIndexNoHyphens += partNumberNoHyphensLength + 1; // +1 for null terminator
    }
    else {
        masterPart->partNumberNoHyphens = partNumber;
        masterPart->partNumberNoHyphensLength = partNumberLength;
    }
    return true;
}
