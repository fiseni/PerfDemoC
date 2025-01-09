#ifndef PROCESSOR_H
#define PROCESSOR_H

const char* get_identifier();
void initialize(MasterPart* masterParts, size_t count, char** partNumbers, size_t partNumbersCount);
size_t run();
char* find_match(char* partNumber);

#endif