#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 1048576

//#define TABLE_SIZE2 1048576
//#define TABLE_SIZE2 524288
//#define TABLE_SIZE2 262144
#define TABLE_SIZE2 131072
//#define TABLE_SIZE2 65536
//#define TABLE_SIZE2 32768
//#define TABLE_SIZE2 16384

// #########################################################
// Hash table storing a string as value.
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

// #########################################################
// Hash table storing a list of strings as value.
typedef struct StringList {
    char** strings;
    size_t count;
    size_t capacity;
} StringList;

typedef struct EntryStringList {
    const char* key;
    StringList* list;
    struct EntryStringList* next;
} EntryStringList;

typedef struct HTableStringList {
    EntryStringList* buckets[TABLE_SIZE2];
} HTableStringList;

HTableStringList* htable_stringlist_create();
const StringList* htable_stringlist_search(HTableStringList* table, const char* key, int keyLength);
void htable_stringlist_add_string(HTableStringList* table, const char* key, int keyLength, const char* value);
void htable_stringlist_free(HTableStringList* table);

// #########################################################
// Hash table storing a list of size_t as value.
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
    EntrySizeList* buckets[TABLE_SIZE2];
} HTableSizeList;

HTableSizeList* htable_sizelist_create();
const SizeList* htable_sizelist_search(HTableSizeList* table, const char* key, int keyLength);
void htable_sizelist_add(HTableSizeList* table, const char* key, int keyLength, size_t value);
void htable_sizelist_free(HTableSizeList* table);

#endif
