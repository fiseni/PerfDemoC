#include <string.h>
#include <stdio.h>
#include "cross_platform_time.h"
#include "common.h"
#include "processor.h"

int main(int argc, char* argv[]) {
	printf("Implementation: %s\n", get_identifier());

	if (argc > 1 && strcmp(argv[1], "test") == 0) {
		run_tests();
	}
	else {
		char* partFile = "data/parts.txt";
		char* masterPartFile = "data/masterParts.txt";
		//char* partFile = "data/partsTest.txt";
		//char* masterPartFile = "data/masterPartsTest.txt";

		if (argc > 1 && strcmp(argv[1], "short") == 0) {
			partFile = "data/partsShort.txt";
			masterPartFile = "data/masterPartsShort.txt";
		}

		double start, end, startProcess, endProcess;
		start = get_time_seconds();

		size_t partNumbersCount = 0;
		char** partNumbers = read_file_lines(partFile, &partNumbersCount);
		size_t masterPartNumbersCount = 0;
		char** masterPartNumbers = read_file_lines(masterPartFile, &masterPartNumbersCount);
		printf("MasterPartNumbers: %zu, PartNumbers: %zu\n", masterPartNumbersCount, partNumbersCount);

		size_t masterPartsCount = 0;
		MasterPart* masterParts = build_masterParts(masterPartNumbers, masterPartNumbersCount, 3, &masterPartsCount);

		startProcess = get_time_seconds();
		initialize(masterParts, masterPartsCount, partNumbers, partNumbersCount);
		printf("Elapsed initialization time: \t%f seconds.\n", get_time_seconds() - startProcess);
		size_t matchCount = run();
		endProcess = get_time_seconds();
		printf("Elapsed processing time: \t%f seconds. Found %zu matches.\n", endProcess - startProcess, matchCount);

		// We don't really need to clean anything here. We're just trying to mimic the actions in the real app.
		for (size_t i = 0; i < masterPartsCount; i++) {
			free(masterParts[i].partNumber);
			masterParts[i].partNumber = NULL;
			free(masterParts[i].partNumberNoHyphens);
			masterParts[i].partNumberNoHyphens = NULL;
		}
		free(masterParts);
		masterParts = NULL;

		for (size_t i = 0; i < masterPartNumbersCount; i++) {
			free(masterPartNumbers[i]);
			masterPartNumbers[i] = NULL;
		}
		for (size_t i = 0; i < partNumbersCount; i++) {
			free(partNumbers[i]);
			partNumbers[i] = NULL;
		}
		free(masterPartNumbers);
		masterPartNumbers = NULL;
		free(partNumbers);
		partNumbers = NULL;

		end = get_time_seconds();
		printf("Elapsed wall time: \t\t%f seconds.\n", end - start);

		return 0;
	}
}