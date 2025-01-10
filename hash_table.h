#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 100

typedef struct Entry {
    const char* key;
    const char* value;
    struct Entry* next;
} Entry;

typedef struct HashTable {
    Entry* buckets[TABLE_SIZE];
} HashTable;

HashTable* create_table();
void insert(HashTable* table, const char* key, const char* value);
void insert_if_not_exists(HashTable* table, const char* key, const char* value);
const char* search(HashTable* table, const char* key);
int delete_entry(HashTable* table, const char* key);
void free_table(HashTable* table);

#endif