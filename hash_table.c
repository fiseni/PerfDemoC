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

HashTable* create_table() {
    HashTable* table = malloc(sizeof(HashTable));
    if (table) {
        for (int i = 0; i < TABLE_SIZE; i++)
            table->buckets[i] = NULL;
    }
    return table;
}

void insert(HashTable* table, const char* key, const char* value) {
    unsigned int index = hash(key);
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

void insert_if_not_exists(HashTable* table, const char* key, const char* value) {
	const char* existing_value = search(table, key);
    if (existing_value) {
        return;
    }

    unsigned int index = hash(key);
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

const char* search(HashTable* table, const char* key) {
    unsigned int index = hash(key);
    Entry* entry = table->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

int delete_entry(HashTable* table, const char* key) {
    unsigned int index = hash(key);
    Entry* entry = table->buckets[index];
    Entry* prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            }
            else {
                table->buckets[index] = entry->next;
            }
            free(entry);
            return 1; // Successfully deleted
        }
        prev = entry;
        entry = entry->next;
    }
    return 0; // Key not found
}

void free_table(HashTable* table) {
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