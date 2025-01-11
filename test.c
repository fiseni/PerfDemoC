#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "source_data.h"
#include "processor.h"
#include "utils.h"

typedef struct PartTest {
	char* expected;
	char* partNumber;
} PartTest;

PartTest partsTest[] =
{
	// expected, partNumber
	{NULL, "  "},
	{NULL, ""},
	{NULL, "a"},
	{NULL, "A"},
	{NULL, "ab"},
	{NULL, "AB"},
	{NULL, "xyz"},
	{NULL, "XYZ"},
	{NULL, "klmn"},
	{NULL, "KLMN"},
	{NULL, "ijkl"},
	{NULL, "IJKL"},
	{"CDEFGHIJKLMNO", "defghijklmno"},
	{"CDEFGHIJKLMNO", "DEFGHIJKLMNO"},
	{"ABCDEFGHI---------------JKLMNO", "DEFGHI---------------JKLMNO"},
	{"ABCDEFGHI---------------JKLMNO", "bcdefghijklmno"},
	{"ABCDEFGHI---------------JKLMNO", "BCDEFGHIJKLMNO"},
	{"AAQWERTYUIO", "AAAAqwertyuio"},
	{"AAQWERTYUIO", "AAAAQWERTYUIO"},
	{"AAQWERTYUIO", "AAQWERTYUIO"},
	{"AQWERTYUIO", "AQWERTYUIO"},
	{"QZP", "qzp"},
	{"ZXC", "zXC"},
	{"SSD-DFF", "SSD-DFF"},
	{"XSSD-DFF", "XSSDDFF"},
};

char* masterPartNumbersTest[] =
{
	"Aqwertyuio",
	"QWERTYUIO",
	"zxc",
	"ZXC",
	"xxabcdefghi-jklmno",
	"ABCDEFGHI---------------JKLMNO",
	"cdefghijklmno",
	"CDEFGHIJKLMNO",
	"0000abcdefghijklmnoX",
	"zx",
	"qzp",
	"QZP",
	"AXqwertyuio",
	"AAqwertyuio",
	"SSDDFF",
	"SSD-DFF",
	"XSSD-DFF",
	"z",
	""
};

static char** extract_partNumbers(size_t count) {
	char** partNumbers = malloc(count * sizeof(char*));
	if (!partNumbers) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < count; i++) {
		partNumbers[i] = partsTest[i].partNumber;
	};
	return partNumbers;
}

void run_tests() {
	size_t masterPartNumbersTestCount = sizeof(masterPartNumbersTest) / sizeof(masterPartNumbersTest[0]);
	size_t partsTestCount = sizeof(partsTest) / sizeof(partsTest[0]);
	char** partNumbers = extract_partNumbers(partsTestCount);
	SourceData* data = data_build(masterPartNumbersTest, masterPartNumbersTestCount, partNumbers, partsTestCount);
	processor_initialize(data);

	for (size_t i = 0; i < partsTestCount; i++) {
		const char* result = processor_find_match(partsTest[i].partNumber);

		if (result == NULL && partsTest[i].expected == NULL) {
			continue;
		}

		if (result != NULL && partsTest[i].expected != NULL && strcmp(result, partsTest[i].expected) == 0) {
			continue;
		}

		printf("Failed at: %s | Expected: %s | Found: %s\n", partsTest[i].partNumber, partsTest[i].expected, result);
		return;
	};

	printf("All tests passed!\n");
}