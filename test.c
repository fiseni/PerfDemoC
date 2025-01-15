#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>	
#include "source_data.h"
#include "processor.h"
#include "utils.h"

static void export_test_data(const char* masterPartsFile, const char* partFile);

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
    char* partsFile = "data/partsTest.txt";
    char* masterPartFile = "data/masterPartsTest.txt";
    export_test_data(masterPartFile, partsFile);

    const SourceData* data = source_data_read(masterPartFile, partsFile);
    processor_initialize(data);

    size_t testPartsCount = sizeof(testParts) / sizeof(testParts[0]);
    for (size_t i = 0; i < testPartsCount; i++) {
        const char* result = processor_find_match(testParts[i].partNumber);

        if (result == NULL && testParts[i].expected == NULL) {
            continue;
        }

        if (result != NULL && testParts[i].expected != NULL && strcmp(result, testParts[i].expected) == 0) {
            continue;
        }

        printf("Failed at: %s | Expected: %s | Found: %s\n\n", testParts[i].partNumber, testParts[i].expected, result);
        return;
    };

    printf("All tests passed!\n\n");
    remove(masterPartFile);
    remove(partsFile);
}

// We're exporting to a file to mimic the entire process.
static void export_test_data(const char* masterPartsFile, const char* partFile) {
    size_t testMasterPartNumbersCount = sizeof(testMasterPartNumbers) / sizeof(testMasterPartNumbers[0]);
    size_t testPartsCount = sizeof(testParts) / sizeof(testParts[0]);
    char** testPartNumbers = malloc(testPartsCount * sizeof(*testPartNumbers));
    CHECK_ALLOC(testPartNumbers);
    for (size_t i = 0; i < testPartsCount; i++) {
        testPartNumbers[i] = testParts[i].partNumber;
    };

    FILE* file1 = fopen(masterPartsFile, "w");
    FILE* file2 = fopen(partFile, "w");
    if (!file1 || !file2) {
        perror("Failed to open files");
        return;
    }
    for (size_t i = 0; i < testMasterPartNumbersCount; i++) {
        fprintf(file1, "%s\n", testMasterPartNumbers[i]);
    }
    fclose(file1);
    for (size_t i = 0; i < testPartsCount; i++) {
        fprintf(file2, "%s\n", testPartNumbers[i]);
    }
    fclose(file2);
}
