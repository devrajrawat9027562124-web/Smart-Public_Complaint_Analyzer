/*
 * hash_table.c
 * Hash table implementation with djb2 hashing and separate chaining.
 */

#include "../../include/engine/hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* ─── djb2 Hash Function ─── */
unsigned long ht_hash(const char* key, int table_size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;  /* hash * 33 + c */
    }
    return hash % table_size;
}

/* Initialize hash table with given number of buckets */
void ht_init(HashTable* ht, int size) {
    ht->size = size;
    ht->count = 0;
    ht->buckets = (HashNode**)calloc(size, sizeof(HashNode*));
    if (!ht->buckets) {
        fprintf(stderr, "Error: Failed to allocate hash table buckets\n");
        return;
    }
}

/* Insert a key-value pair (overwrites if key exists) */
void ht_insert(HashTable* ht, const char* key, int value) {
    unsigned long index = ht_hash(key, ht->size);
    
    /* Check if key already exists — update value */
    HashNode* current = ht->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            return;
        }
        current = current->next;
    }
    
    /* Key not found — prepend new node to bucket's linked list */
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (!new_node) {
        fprintf(stderr, "Error: Failed to allocate hash node\n");
        return;
    }
    strncpy(new_node->key, key, sizeof(new_node->key) - 1);
    new_node->key[sizeof(new_node->key) - 1] = '\0';
    new_node->value = value;
    new_node->next = ht->buckets[index];  /* Prepend */
    ht->buckets[index] = new_node;
    ht->count++;
}

/* Search for a key, returns its value or -1 if not found */
int ht_search(HashTable* ht, const char* key) {
    unsigned long index = ht_hash(key, ht->size);
    HashNode* current = ht->buckets[index];
    
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return -1;  /* Not found */
}

/* Check if a key exists (1 = yes, 0 = no) */
int ht_contains(HashTable* ht, const char* key) {
    return ht_search(ht, key) != -1;
}

/* Remove a key-value pair */
int ht_remove(HashTable* ht, const char* key) {
    unsigned long index = ht_hash(key, ht->size);
    HashNode* current = ht->buckets[index];
    HashNode* prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                ht->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            ht->count--;
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;  /* Not found */
}

/* Free all memory used by the hash table */
void ht_free(HashTable* ht) {
    if (!ht->buckets) return;
    
    for (int i = 0; i < ht->size; i++) {
        HashNode* current = ht->buckets[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ht->buckets);
    ht->buckets = NULL;
    ht->count = 0;
    ht->size = 0;
}
