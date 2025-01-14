#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

// Based on the requirements the part numbers are less than 50 characters (ASCII).
// Defining the max as 50 makes it easier to work with arrays and buffer sizes (null terminator).
#define MAX_STRING_LENGTH ((size_t)50)

// Based on the requirements we should ignore part numbers with less than 3 characters.
#define MIN_STRING_LENGTH ((size_t)3)

typedef struct MasterPart {
    const char* partNumber;
    const char* partNumberNoHyphens;
    size_t partNumberLength;
    size_t partNumberNoHyphensLength;
} MasterPart;

typedef struct Part {
    const char* partNumber;
    size_t partNumberLength;
} Part;

typedef struct SourceData {
    const MasterPart* masterParts;
    size_t masterPartsCount;
    const Part* parts;
    size_t partsCount;
} SourceData;

const SourceData* data_build(char** masterPartNumbers, size_t masterPartNumbersCount, char** partNumbers, size_t partNumbersCount);
const SourceData* data_read(int argc, char* argv[]);
void data_print(const SourceData* data);

int compare_mp_by_partNumber_length_asc(const void* a, const void* b);
int compare_mp_by_partNumber_length_desc(const void* a, const void* b);
int compare_mp_by_partNumberNoHyphens_length_asc(const void* a, const void* b);
int compare_part_by_partNumber_length_asc(const void* a, const void* b);

#endif
