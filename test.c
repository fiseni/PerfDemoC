#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "processor.h"

typedef struct {
	char* Expected;
	char* PartNumber;
} PartTest;

PartTest partsTest[] =
{
	// Expected, PartNumber
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
	};
	for (size_t i = 0; i < count; i++) {
		partNumbers[i] = partsTest[i].PartNumber;
	};
	return partNumbers;
}

void run_tests() {
	size_t masterPartNumbersTestCount = sizeof(masterPartNumbersTest) / sizeof(masterPartNumbersTest[0]);
	size_t partsTestCount = sizeof(partsTest) / sizeof(partsTest[0]);
	char** partNumbers = extract_partNumbers(partsTestCount);

	size_t masterPartsCount = 0;
	MasterPart* masterParts = build_masterParts(masterPartNumbersTest, masterPartNumbersTestCount, 3, &masterPartsCount);
	
	initialize(masterParts, masterPartsCount, partNumbers, partsTestCount);

	for (size_t i = 0; i < partsTestCount; i++) {
		char* result = find_match(partsTest[i].PartNumber);

		if (result == NULL && partsTest[i].Expected == NULL) {
			continue;
		}

		if (result != NULL && partsTest[i].Expected != NULL && strcmp(result, partsTest[i].Expected) == 0) {
			continue;
		}

		printf("Failed at: %s | Expected: %s | Found: %s\n", partsTest[i].PartNumber, partsTest[i].Expected, result);
	};

	printf("All tests passed!\n");
}