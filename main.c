#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cross_platform_time.h"
#include "source_data.h"
#include "processor.h"

void run_tests();

int main(int argc, char* argv[]) {
    printf("\nImplementation: %s\n\n", processor_get_identifier());

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        run_tests();
    }
    else {
        double start = time_get_seconds();
        const SourceData* data = data_read(argc, argv);
        printf("MasterParts Count: \t%zu\n", data->masterPartsCount);
        printf("Parts Count: \t\t%zu\n\n", data->partsCount);
        printf("Reading source data: \t\t%f seconds.\n", time_get_seconds() - start);

        double start2 = time_get_seconds();
        processor_initialize(data);
        printf("Processor initialization: \t%f seconds.\n", time_get_seconds() - start2);

        double start3 = time_get_seconds();
        size_t matchCount = 0;
        const char* result = NULL;
        for (size_t i = 0; i < data->partsCount; i++) {
            result = processor_find_match(data->parts[i].partNumber);
            if (result) {
                matchCount++;
            }
            //printf("PartNumber: %30s %30s\n", data->partNumbers[i], result);
        };

        printf("Processor matching: \t\t%f seconds. Found %zu matches.\n", time_get_seconds() - start3, matchCount);
        // We don't really need to clean anything here, the app is terminating.
        //processor_clean();
        printf("Processor wall time: \t\t%f seconds.\n", time_get_seconds() - start2);
        printf("Wall time: \t\t\t%f seconds.\n", time_get_seconds() - start);
        return 0;
    }
}
