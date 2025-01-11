#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

// It's provided that the length of part numbers is maximum 50 characters
#define MAX_LINE_LEN ((size_t)50)

typedef struct MasterPart {
	char* partNumber;
	char* partNumberNoHyphens;
	int partNumberLength;
	int partNumberNoHyphensLength;
} MasterPart;

typedef struct Part {
	char* partNumber;
	int partNumberLength;
} Part;

typedef struct SourceData {
	MasterPart* masterParts;
	size_t masterPartsCount;
	char** partNumbers;
	size_t partNumbersCount;
} SourceData;

SourceData* data_build(char** masterPartNumbers, size_t masterPartNumbersCount, char** partNumbers, size_t partNumbersCount);
SourceData* data_read(int argc, char* argv[]);
void data_clean(SourceData* data);
void data_print(SourceData* data);

#endif
