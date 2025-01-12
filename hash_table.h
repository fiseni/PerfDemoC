#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 1048576

typedef struct EntryString {
    const char* key;
    const char* value;
    struct EntryString* next;
} EntryString;

typedef struct HTableString {
    EntryString* buckets[TABLE_SIZE];
} HTableString;

HTableString* htable_string_create();
const char* htable_string_search(HTableString* table, const char* key, int keyLength);
void htable_string_insert_if_not_exists(HTableString* table, const char* key, int keyLength, const char* value);
void htable_string_free(HTableString* table);

#endif