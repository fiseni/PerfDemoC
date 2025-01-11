#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash_table.h"

static unsigned int hash(const char* key) {
    unsigned int hash = 0;
    while (*key)
        hash = (hash * 31) + *key++;
    return hash % TABLE_SIZE;
}

HashTable* htable_string_create() {
    HashTable* table = malloc(sizeof(HashTable));
    if (table) {
        for (int i = 0; i < TABLE_SIZE; i++)
            table->buckets[i] = NULL;
    }
    return table;
}

// Used to avoid re-computing the hash value.
static const char* search_internal(HashTable* table, const char* key, unsigned int index) {
    Entry* entry = table->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const char* htable_string_search(HashTable* table, const char* key) {
    unsigned int index = hash(key);
    return search_internal(table, key, index);
}

void htable_string_insert_if_not_exists(HashTable* table, const char* key, const char* value) {
    unsigned int index = hash(key);
    const char* existing_value = search_internal(table, key, index);
    if (existing_value) {
        return;
    }

    Entry* new_entry = malloc(sizeof(Entry));
    if (!new_entry) {
        fprintf(stderr, "Memory allocation failed for new entry.\n");
        return;
    }
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
}

void htable_string_free(HashTable* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry* entry = table->buckets[i];
        while (entry) {
            Entry* temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(table);
}