#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hash_table.h"

static unsigned int hash(const char* key, int keyLength) {
    unsigned int hash = 0;
    for (int i = 0; i < keyLength; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash & (TABLE_SIZE - 1);
}

static SizeList* sizelist_create() {
    SizeList* list = malloc(sizeof(*list));
    assert(list);
    list->values = malloc(8 * sizeof(size_t));
    list->count = 0;
    list->capacity = 8;
    return list;
}

static void sizelist_add(SizeList* list, size_t value) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        size_t* newList = realloc(list->values, list->capacity * sizeof(size_t));
        assert(newList);
        list->values = newList;
    }
    list->values[list->count++] = value;
}

HTableSizeList* htable_sizelist_create() {
    HTableSizeList* table = malloc(sizeof(*table));
    assert(table);
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    return table;
}

const SizeList* htable_sizelist_search(HTableSizeList* table, const char* key, int keyLength) {
    unsigned int index = hash(key, keyLength);
    EntrySizeList* entry = table->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->list;
        }
        entry = entry->next;
    }
    return NULL;
}

void htable_sizelist_add(HTableSizeList* table, const char* key, int keyLength, size_t value) {
    unsigned int index = hash(key, keyLength);
    EntrySizeList* entry = table->buckets[index];

    // Search for existing key
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
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
    assert(newEntry);
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
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        EntrySizeList* entry = table->buckets[i];
        while (entry) {
            EntrySizeList* temp = entry;
            entry = entry->next;
            sizelist_free(temp->list);
            free(temp);
        }
    }
    free(table);
}
