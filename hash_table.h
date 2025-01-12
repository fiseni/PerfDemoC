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

//#define TABLE_SIZE2 1048576
//#define TABLE_SIZE2 524288
#define TABLE_SIZE2 262144
//#define TABLE_SIZE2 131072
//#define TABLE_SIZE2 65536

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
    EntryStringList* buckets[TABLE_SIZE];
} HTableStringList;

HTableStringList* htable_stringlist_create();
const StringList* htable_stringlist_search(HTableStringList* table, const char* key, int keyLength);
void htable_stringlist_add_string(HTableStringList* table, const char* key, int keyLength, const char* value);
void htable_stringlist_free(HTableStringList* table);

#endif