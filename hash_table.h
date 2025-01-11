#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 1000000

typedef struct Entry {
    const char* key;
    const char* value;
    struct Entry* next;
} Entry;

typedef struct HashTable {
    Entry* buckets[TABLE_SIZE];
} HashTable;

HashTable* htable_string_create();
const char* htable_string_search(HashTable* table, const char* key);
void htable_string_insert_if_not_exists(HashTable* table, const char* key, const char* value);
void htable_string_free(HashTable* table);

#endif