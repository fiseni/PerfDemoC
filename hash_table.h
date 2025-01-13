#ifndef HASH_TABLE_H
#define HASH_TABLE_H

// #########################################################
// Hash table storing a string as value.
typedef struct EntryString {
    const char* key;
    const char* value;
    struct EntryString* next;
} EntryString;

typedef struct HTableString {
    EntryString** buckets;
    size_t size;
    EntryString* block;
    size_t blockCount;
    size_t blockIndex;
} HTableString;

HTableString* htable_string_create(size_t size);
const char* htable_string_search(HTableString* table, const char* key, int keyLength);
void htable_string_insert_if_not_exists(HTableString* table, const char* key, int keyLength, const char* value);
void htable_string_free(HTableString* table);

// #########################################################
// Hash table storing a list of size_t as value.
#define TABLE_SIZE 131072

typedef struct SizeList {
    size_t* values;
    size_t count;
    size_t capacity;
} SizeList;

typedef struct EntrySizeList {
    const char* key;
    SizeList* list;
    struct EntrySizeList* next;
} EntrySizeList;

typedef struct HTableSizeList {
    EntrySizeList* buckets[TABLE_SIZE];
} HTableSizeList;

HTableSizeList* htable_sizelist_create();
const SizeList* htable_sizelist_search(HTableSizeList* table, const char* key, int keyLength);
void htable_sizelist_add(HTableSizeList* table, const char* key, int keyLength, size_t value);
void htable_sizelist_free(HTableSizeList* table);

#endif
