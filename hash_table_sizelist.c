#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"

static size_t hash(const HTableSizeList* table, const char* key, size_t keyLength) {
    size_t hash = 0x811C9DC5; // 2166136261
    for (size_t i = 0; i < keyLength; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash & (table->size - 1);
}

static bool is_equal(const char* str1, const char* str2, size_t str2Length) {
    for (size_t i = 0; i < str2Length; i++) {
        if (str1[i] == '\0' || str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

static void linked_list_add(HTableSizeList* table, EntrySizeList* entry, size_t value) {
    ListItem* newItem;
    if (table->blockIndex < table->blockCount) {
        newItem = &table->block[table->blockIndex++];
    }
    else {
        newItem = malloc(sizeof(*newItem));
        CHECK_ALLOC(newItem);
    }

    newItem->value = value;
    newItem->next = entry->list;
    entry->list = newItem;
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
    table->buckets = malloc(sizeof(*table->buckets) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }
    table->block = malloc(sizeof(*table->block) * size);
    CHECK_ALLOC(table->block);
    table->blockCount = size;
    table->blockIndex = 0;
    return table;
}

const ListItem* htable_sizelist_search(const HTableSizeList* table, const char* key, size_t keyLength) {
    size_t index = hash(table, key, keyLength);
    EntrySizeList* entry = table->buckets[index];
    while (entry) {
        if (is_equal(entry->key, key, keyLength)) {
            return entry->list;
        }
        entry = entry->next;
    }
    return NULL;
}

void htable_sizelist_add(HTableSizeList* table, const char* key, size_t keyLength, size_t value) {
    size_t index = hash(table, key, keyLength);
    EntrySizeList* entry = table->buckets[index];

    while (entry) {
        if (is_equal(entry->key, key, keyLength)) {
            linked_list_add(table, entry, value);
            return;
        }
        entry = entry->next;
    }

    // Key not found, create new entry
    EntrySizeList* newEntry = malloc(sizeof(*newEntry));
    CHECK_ALLOC(newEntry);
    newEntry->key = key;
    newEntry->list = NULL;
    linked_list_add(table, newEntry, value);
    newEntry->next = table->buckets[index];
    table->buckets[index] = newEntry;
}

void htable_sizelist_free(HTableSizeList* table) {
    for (size_t i = 0; i < table->size; i++) {
        EntrySizeList* entry = table->buckets[i];
        while (entry) {
            EntrySizeList* temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(table->block);
    free(table->buckets);
    free(table);
}
