#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

// It's provided that the length of part numbers is maximum 50 characters
#define MAX_LINE_LEN ((size_t)50)

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
