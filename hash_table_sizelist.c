#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"

static uint32_t hash(HTableSizeList* table, const char* key, int keyLength) {
    uint32_t hash = 0x811C9DC5; // 2166136261
    for (int i = 0; i < keyLength; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash & (table->size - 1);
}

static bool is_equal(const char* str1, const char* str2, int str2Length) {
    for (int i = 0; i < str2Length; i++) {
        if (str1[i] == '\0' || str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

static SizeList* sizelist_create() {
    SizeList* list = malloc(sizeof(*list));
    CHECK_ALLOC(list);
    list->values = malloc(8 * sizeof(size_t));
    list->count = 0;
    list->capacity = 8;
    return list;
}

static void sizelist_add(SizeList* list, size_t value) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        size_t* newList = realloc(list->values, list->capacity * sizeof(size_t));
        CHECK_ALLOC(newList);
        list->values = newList;
    }
    list->values[list->count++] = value;
}

HTableSizeList* htable_sizelist_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTableSizeList* table = malloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = malloc(sizeof(EntrySizeList*) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }
    return table;
}

const SizeList* htable_sizelist_search(HTableSizeList* table, const char* key, int keyLength) {
    unsigned int index = hash(table, key, keyLength);
    EntrySizeList* entry = table->buckets[index];
    while (entry) {
        //if (strcmp(entry->key, key) == 0) {
        if (is_equal(entry->key, key, keyLength)) {
            return entry->list;
        }
        entry = entry->next;
    }
    return NULL;
}

void htable_sizelist_add(HTableSizeList* table, const char* key, int keyLength, size_t value) {
    unsigned int index = hash(table, key, keyLength);
    EntrySizeList* entry = table->buckets[index];

    // Search for existing key
    while (entry) {
        if (is_equal(entry->key, key, keyLength)) {
            // Key exists, add value if not already present
            for (size_t i = 0; i < entry->list->count; i++) {
                if (entry->list->values[i] == value) {
                    return; // Value already exists
                }
            }
            sizelist_add(entry->list, value);
            return;
        }
        entry = entry->next;
    }

    // Key not found, create new entry
    EntrySizeList* newEntry = malloc(sizeof(*newEntry));
    CHECK_ALLOC(newEntry);
    newEntry->key = key;
    newEntry->list = sizelist_create();
    sizelist_add(newEntry->list, value);
    newEntry->next = table->buckets[index];
    table->buckets[index] = newEntry;
}

static void sizelist_free(SizeList* list) {
    free(list->values);
    free(list);
}

void htable_sizelist_free(HTableSizeList* table) {
    for (size_t i = 0; i < table->size; i++) {
        EntrySizeList* entry = table->buckets[i];
        while (entry) {
            EntrySizeList* temp = entry;
            entry = entry->next;
            sizelist_free(temp->list);
            free(temp);
        }
    }
    free(table->buckets);
    free(table);
}
