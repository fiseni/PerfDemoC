#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cross_platform_time.h"
#include "source_data.h"
#include "test.h"
#include "processor.h"

typedef struct Args {
    char* masterPartsFilename;
    char* partsFilename;
    int isTestRun;
} Args;

static void run(const SourceData* data) {
    double start = time_get_seconds();
    processor_initialize(data);
    printf("Processor initialization: \t%f seconds.\n", time_get_seconds() - start);

    double start2 = time_get_seconds();
    size_t matchCount = 0;
    for (size_t i = 0; i < data->partsCount; i++) {
        if (processor_find_match(data->parts[i].partNumber)) {
            matchCount++;
        }
    };

    printf("Processor matching: \t\t%f seconds. Found %zu matches.\n", time_get_seconds() - start2, matchCount);
    processor_clean();
}

static Args parse_arguments(int argc, char* argv[]) {
#if _DEBUG
    //return (Args) { "data/masterPartsTest.txt", "data/partsTest.txt", 0 };
    //return (Args) { "data/masterParts.txt", "data/parts.txt", 0 };
    return (Args) { "data/masterPartsShort.txt", "data/partsShort.txt", 0 };
#endif
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return (Args) { NULL, NULL, 1 };
    }
    if (argc < 3) {
        fprintf(stderr, "\nUsage: \t\t%30s <masterPartsFile> <partsFile>\n", argv[0]);
        fprintf(stderr, "Usage for test: %s test\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    return (Args) { argv[1], argv[2], 0 };
}

int main(int argc, char* argv[]) {
    Args args = parse_arguments(argc, argv);
    printf("\nImplementation: %s\n\n", processor_get_identifier());

    if (args.isTestRun) {
        run_tests();
        return 0;
    }

    double start = time_get_seconds();
    const SourceData* data = source_data_read(args.masterPartsFilename, args.partsFilename);
    printf("MasterParts Count: \t%zu\n", data->masterPartsCount);
    printf("Parts Count: \t\t%zu\n\n", data->partsCount);
    printf("Reading source data: \t\t%f seconds.\n", time_get_seconds() - start);

    double start2 = time_get_seconds();
    run(data);
    printf("Processor wall time: \t\t%f seconds.\n", time_get_seconds() - start2);

    source_data_clean(data);
    printf("Wall time: \t\t\t%f seconds.\n\n", time_get_seconds() - start);
    return 0;
}
