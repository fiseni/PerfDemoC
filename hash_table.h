#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 1000000

typedef struct EntryString {
    const char* key;
    const char* value;
    struct EntryString* next;
} EntryString;

typedef struct HTableString {
    EntryString* buckets[TABLE_SIZE];
} HTableString;

HTableString* htable_string_create();
const char* htable_string_search(HTableString* table, const char* key);
void htable_string_insert_if_not_exists(HTableString* table, const char* key, const char* value);
void htable_string_free(HTableString* table);

#endif