#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.h"
#include "hash_table.h"

static uint32_t hash(const HTableString* table, const char* key, int keyLength) {
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

HTableString* htable_string_create(size_t size) {
    size_t tableSize = next_power_of_two(size);
    if (tableSize == 0) {
        // Some default powerOfTwo value in case of overflow.
        tableSize = 32;
    }
    HTableString* table = malloc(sizeof(*table));
    CHECK_ALLOC(table);
    table->size = tableSize;
    table->buckets = malloc(sizeof(EntryString*) * tableSize);
    CHECK_ALLOC(table->buckets);
    for (size_t i = 0; i < tableSize; i++) {
        table->buckets[i] = NULL;
    }
    table->block = malloc(sizeof(EntryString) * size);
    CHECK_ALLOC(table->block);
    table->blockCount = size;
    table->blockIndex = 0;
    return table;
}

// Used to avoid re-computing the hash value.
static const char* search_internal(const HTableString* table, const char* key, int keyLength, unsigned int index) {
    EntryString* entry = table->buckets[index];
    while (entry) {
        //if (strcmp(entry->key, key) == 0) {
        //if (strncmp(entry->key, key, keyLength) == 0) {
        if (is_equal(entry->key, key, keyLength)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const char* htable_string_search(const HTableString* table, const char* key, int keyLength) {
    unsigned int index = hash(table, key, keyLength);
    return search_internal(table, key, keyLength, index);
}

void htable_string_insert_if_not_exists(HTableString* table, const char* key, int keyLength, const char* value) {
    unsigned int index = hash(table, key, keyLength);
    const char* existing_value = search_internal(table, key, keyLength, index);
    if (existing_value) {
        return;
    }

    EntryString* new_entry;
    if (table->blockIndex < table->blockCount) {
        new_entry = &table->block[table->blockIndex++];
        new_entry->allocatedFromBlock = 1;
    }
    else {
        new_entry = malloc(sizeof(EntryString));
        CHECK_ALLOC(new_entry);
        new_entry->allocatedFromBlock = 0;
    }

    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

static void free_entries(HTableString* table) {
    for (size_t i = 0; i < table->size; i++) {
        EntryString* entry = table->buckets[i];
        while (entry) {
            if (entry->allocatedFromBlock == 0) {
                EntryString* temp = entry;
                entry = entry->next;
                free(temp);
            }
            else {
                entry = entry->next;
            }
        }
    }
}

void htable_string_free(HTableString* table) {
    // We're providing the appropriate size for the tables everywhere
    // and malloc for individual entries will never occur.
    // free_entries(table);
    free(table->block);
    free(table->buckets);
    free(table);
}
