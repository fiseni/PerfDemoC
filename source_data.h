#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

// Based on the requirements the part numbers are less than 50 characters (ASCII).
// Defining the max as 50 makes it easier to work with arrays and buffer sizes (null terminator).
#define MAX_STRING_LENGTH ((size_t)50)

// Based on the requirements we should ignore part numbers with less than 3 characters.
#define MIN_STRING_LENGTH ((size_t)3)

typedef struct MasterPart {
    const char *partNumber;
    const char *partNumberNoHyphens;
    size_t partNumberLength;
    size_t partNumberNoHyphensLength;
    size_t index;
} MasterPart;

typedef struct Part {
    const char *partNumber;
    size_t partNumberLength;
    size_t index;
} Part;

typedef struct SourceData {
    const MasterPart *masterParts;
    size_t masterPartsCount;
    const Part *parts;
    size_t partsCount;
} SourceData;

const SourceData *source_data_read(const char *masterPartsFilename, const char *partsFilename);
void source_data_clean(const SourceData *data);

#endif
