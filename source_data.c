#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "utils.h"
#include "cross_platform_time.h"
#include "source_data.h"

static Part* build_parts(const char* partsPath, size_t* outCount);
static MasterPart* build_masterParts(const char* masterPartsPath, size_t* outCount);
static bool populate_masterPart(MasterPart* masterPart, char* partNumber, size_t length);

const SourceData* source_data_read(const char* masterPartsPath, const char* partsPath) {
    size_t masterPartsCount = 0;
    MasterPart* masterParts = build_masterParts(masterPartsPath, &masterPartsCount);
    size_t partsCount = 0;
    Part* parts = build_parts(partsPath, &partsCount);

    SourceData* data = (SourceData*)malloc(sizeof(*data));
    CHECK_ALLOC(data);
    data->masterParts = masterParts;
    data->masterPartsCount = masterPartsCount;
    data->parts = parts;
    data->partsCount = partsCount;

    return data;
}

void source_data_clean(SourceData* data) {
    // All strings are allocated from a single block
    free((void*)data->masterParts->partNumber);
    free((void*)data->parts->partNumber);

    free((void*)data->masterParts);
    free((void*)data->parts);
    free((void*)data);
}

static Part* build_parts(const char* partsPath, size_t* outCount) {
    size_t fileSize = get_file_size_bytes(partsPath);
    FILE* file = fopen(partsPath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open parts file: %s\n", partsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * MAX_STRING_LENGTH * fileSize;
    char* block = malloc(blockSize);
    CHECK_ALLOC(block);

    size_t blockIndex = 0;
    size_t lineCount = 0;
    size_t bytes_read;
    size_t bufferSize = 16000 < fileSize ? 16000 : fileSize;

    char* buffer = &block[blockIndex];
    while ((bytes_read = fread(buffer, 1, bufferSize, file)) > 0) {
        for (size_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                lineCount++;
            }
        }
        blockIndex += bytes_read;
        if (blockIndex + bufferSize > blockSize) {
            fprintf(stderr, "Buffer overflow detected while reading parts.\n");
            free(block);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        buffer = &block[blockIndex];
    }
    // Handle the case where the last line might not end with a newline
    if (blockIndex > 0 && block[blockIndex - 1] != '\n') {
        lineCount++;
    }

    Part* parts = malloc(lineCount * sizeof(*parts));
    CHECK_ALLOC(parts);

    size_t stringStartIndex = 0;
    size_t partsIndex = 0;
    for (size_t i = 0; i < blockIndex; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            parts[partsIndex].partNumber = &block[stringStartIndex];
            parts[partsIndex].partNumberLength = i - stringStartIndex;
            partsIndex++;
            stringStartIndex = i + 1;
        }
    }

    // Handle the last line if it doesn't end with a newline
    if (stringStartIndex < blockIndex) {
        parts[partsIndex].partNumber = &block[stringStartIndex];
        parts[partsIndex].partNumberLength = blockIndex - stringStartIndex;
        partsIndex++;
    }

    fclose(file);
    *outCount = partsIndex;
    return parts;
}

static MasterPart* build_masterParts(const char* masterPartsPath, size_t* outCount) {
    size_t fileSize = get_file_size_bytes(masterPartsPath);
    FILE* file = fopen(masterPartsPath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", masterPartsPath);
        exit(EXIT_FAILURE);
    }

    size_t blockSize = sizeof(char) * MAX_STRING_LENGTH * fileSize;
    char* block = malloc(blockSize);
    CHECK_ALLOC(block);

    size_t blockIndex = 0;
    size_t lineCount = 0;
    size_t bytes_read;
    size_t bufferSize = 16000 < fileSize ? 16000 : fileSize;

    char* buffer = &block[blockIndex];
    while ((bytes_read = fread(buffer, 1, bufferSize, file)) > 0) {
        for (size_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                lineCount++;
            }
        }
        blockIndex += bytes_read;
        if (blockIndex + bufferSize > blockSize) {
            fprintf(stderr, "Buffer overflow detected while reading masterParts.\n");
            free(block);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        buffer = &block[blockIndex];
    }
    // Handle the case where the last line might not end with a newline
    if (blockIndex > 0 && block[blockIndex - 1] != '\n') {
        lineCount++;
    }

    MasterPart* masterParts = malloc(lineCount * sizeof(*masterParts));
    CHECK_ALLOC(masterParts);

    size_t stringStartIndex = 0;
    size_t masterPartsIndex = 0;
    for (size_t i = 0; i < blockIndex; i++) {
        if (block[i] == '\n') {
            block[i] = '\0';
            size_t length = i - stringStartIndex;
            if (populate_masterPart(&masterParts[masterPartsIndex], &block[stringStartIndex], length)) {
                masterPartsIndex++;
            }
            stringStartIndex = i + 1;
        }
    }

    // Handle the last line if it doesn't end with a newline
    if (stringStartIndex < blockIndex) {
        size_t length = blockIndex - stringStartIndex;
        if (populate_masterPart(&masterParts[masterPartsIndex], &block[stringStartIndex], length)) {
            masterPartsIndex++;
        }
    }

    fclose(file);
    *outCount = masterPartsIndex;
    return masterParts;
}

static bool populate_masterPart(MasterPart* masterPart, char* partNumber, size_t length) {
    if (length < MIN_STRING_LENGTH) {
        return false;
    }
    size_t partNumberLength;
    str_to_upper_trim_in_place(partNumber, length, &partNumberLength);

    masterPart->partNumber = partNumber;
    masterPart->partNumberLength = partNumberLength;

    if (str_contains_dash(partNumber, partNumberLength)) {
        char* partNumberNoHyphens = malloc(partNumberLength);
        size_t partNumberNoHyphensLength;
        str_remove_char(partNumber, partNumberLength, partNumberNoHyphens, partNumberLength, '-', &partNumberNoHyphensLength);
        masterPart->partNumberNoHyphens = partNumberNoHyphens;
        masterPart->partNumberNoHyphensLength = partNumberNoHyphensLength;
    }
    else {
        masterPart->partNumberNoHyphens = partNumber;
        masterPart->partNumberNoHyphensLength = partNumberLength;
    }
    return true;
}
