#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>	
#include "source_data.h"
#include "processor.h"
#include "utils.h"

typedef struct TestPart {
    char* expected;
    char* partNumber;
} TestPart;

TestPart testParts[] =
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
    {"699", "W50-699"},
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

char* testMasterPartNumbers[] =
{
    "699",
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

void run_tests() {
    size_t testMasterPartNumbersCount = sizeof(testMasterPartNumbers) / sizeof(testMasterPartNumbers[0]);
    size_t testPartsCount = sizeof(testParts) / sizeof(testParts[0]);
    char** testPartNumbers = malloc(testPartsCount * sizeof(char*));
    assert(testPartNumbers);
    for (size_t i = 0; i < testPartsCount; i++) {
        testPartNumbers[i] = testParts[i].partNumber;
    };

    SourceData* data = data_build(testMasterPartNumbers, testMasterPartNumbersCount, testPartNumbers, testPartsCount);
    processor_initialize(data);

    for (size_t i = 0; i < testPartsCount; i++) {
        const char* result = processor_find_match(testParts[i].partNumber);

        if (result == NULL && testParts[i].expected == NULL) {
            continue;
        }

        if (result != NULL && testParts[i].expected != NULL && strcmp(result, testParts[i].expected) == 0) {
            continue;
        }

        printf("Failed at: %s | Expected: %s | Found: %s\n", testParts[i].partNumber, testParts[i].expected, result);
        return;
    };

    printf("All tests passed!\n");
}
