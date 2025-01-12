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

HTableString* htable_string_create() {
    HTableString* table = malloc(sizeof(HTableString));
    assert(table);
    for (int i = 0; i < TABLE_SIZE; i++)
        table->buckets[i] = NULL;
    return table;
}

// Used to avoid re-computing the hash value.
static const char* search_internal(HTableString* table, const char* key, unsigned int index) {
    EntryString* entry = table->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const char* htable_string_search(HTableString* table, const char* key, int keyLength) {
    unsigned int index = hash(key, keyLength);
    return search_internal(table, key, index);
}

void htable_string_insert_if_not_exists(HTableString* table, const char* key, int keyLength, const char* value) {
    unsigned int index = hash(key, keyLength);
    const char* existing_value = search_internal(table, key, index);
    if (existing_value) {
        return;
    }


    EntryString* new_entry = malloc(sizeof(EntryString));
    assert(new_entry);
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

void htable_string_free(HTableString* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        EntryString* entry = table->buckets[i];
        while (entry) {
            EntryString* temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(table);
}
