#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hash_table.h"

//static unsigned int hash(const char* key) {
//	unsigned int hash = 0;
//	while (*key)
//		hash = (hash * 31) + *key++;
//	return hash & (TABLE_SIZE - 1);
//}

static unsigned int hash(const char* key, int keyLength) {
	unsigned int hash = 0;
	for (int i = 0; i < keyLength; i++) {
		hash = (hash * 31) + key[i];
	}
	return hash & (TABLE_SIZE - 1);
}

static StringList* stringlist_create() {
	StringList* list = malloc(sizeof(StringList));
	assert(list);
	list->strings = malloc(4 * sizeof(char*)); // Initial capacity: 4
	list->count = 0;
	list->capacity = 4;
	return list;
}

static void stringlist_add(StringList* list, const char* value) {
	if (list->count >= list->capacity) {
		list->capacity *= 2;
		char** newList = realloc(list->strings, list->capacity * sizeof(char*));
		assert(newList);
		list->strings = newList;
	}
	list->strings[list->count++] = value;
}

HTableStringList* htable_stringlist_create() {
	HTableStringList* table = malloc(sizeof(HTableStringList));
	assert(table);
	for (size_t i = 0; i < TABLE_SIZE; i++) {
		table->buckets[i] = NULL;
	}
	return table;
}

const StringList* htable_stringlist_search(HTableStringList* table, const char* key, int keyLength) {
	unsigned int index = hash(key, keyLength);
	EntryStringList* entry = table->buckets[index];
	while (entry) {
		if (strcmp(entry->key, key) == 0) {
			return entry->list;
		}
		entry = entry->next;
	}
	return NULL;
}

void htable_stringlist_add_string(HTableStringList* table, const char* key, int keyLength, const char* value) {
	unsigned int index = hash(key, keyLength);
	EntryStringList* entry = table->buckets[index];

	// Search for existing key
	while (entry) {
		if (strcmp(entry->key, key) == 0) {
			// Key exists, add value if not already present
			for (size_t i = 0; i < entry->list->count; i++) {
				if (strcmp(entry->list->strings[i], value) == 0) {
					return; // Value already exists
				}
			}
			stringlist_add(entry->list, value);
			return;
		}
		entry = entry->next;
	}

	// Key not found, create new entry
	EntryStringList* newEntry = malloc(sizeof(EntryStringList));
	assert(newEntry);
	newEntry->key = key;
	newEntry->list = stringlist_create();
	stringlist_add(newEntry->list, value);
	newEntry->next = table->buckets[index];
	table->buckets[index] = newEntry;
}

static void stringlist_free(StringList* list) {
	for (size_t i = 0; i < list->count; i++) {
		free(list->strings[i]);
	}
	free(list->strings);
	free(list);
}

void htable_stringlist_free(HTableStringList* table) {
	for (size_t i = 0; i < TABLE_SIZE; i++) {
		EntryStringList* entry = table->buckets[i];
		while (entry) {
			EntryStringList* temp = entry;
			entry = entry->next;
			if (temp && temp->key) {
				free((void*)temp->key);
			}
			stringlist_free(temp->list);
			free(temp);
		}
	}
	free(table);
}

//// Example usage
//int main() {
//    HTableStringList* table = htable_stringlist_create();
//
//    htable_stringlist_insert_if_not_exists(table, "key1", "value1");
//    htable_stringlist_insert_if_not_exists(table, "key1", "value2");
//    htable_stringlist_insert_if_not_exists(table, "key2", "value3");
//
//    const StringList* list = htable_stringlist_search_list(table, "key1");
//    if (list) {
//        printf("Values for key1:\n");
//        for (size_t i = 0; i < list->count; i++) {
//            printf("%s\n", list->strings[i]);
//        }
//    }
//    else {
//        printf("Key not found\n");
//    }
//
//    htable_stringlist_free(table);
//    return 0;
//}