/*
 * hash_table.h
 * Hash table with separate chaining (linked lists) for collision resolution.
 * Used by the NLP analyzer for O(1) keyword lookups.
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Hash node — linked list node for separate chaining */
typedef struct HashNode {
    char key[50];
    int value;
    struct HashNode* next;
} HashNode;

/* Hash table — array of bucket pointers */
typedef struct {
    HashNode** buckets;
    int size;
    int count;
} HashTable;

/* ─── Operations ─── */

/* Initialize hash table with given number of buckets */
void ht_init(HashTable* ht, int size);

/* Insert a key-value pair (overwrites existing key) */
void ht_insert(HashTable* ht, const char* key, int value);

/* Search for a key, returns its value or -1 if not found */
int ht_search(HashTable* ht, const char* key);

/* Check if a key exists (1 = yes, 0 = no) */
int ht_contains(HashTable* ht, const char* key);

/* Remove a key-value pair (returns 1 if found and removed, 0 otherwise) */
int ht_remove(HashTable* ht, const char* key);

/* Free all memory used by the hash table */
void ht_free(HashTable* ht);

/* Hash function (djb2) */
unsigned long ht_hash(const char* key, int table_size);

#ifdef __cplusplus
}
#endif

#endif /* HASH_TABLE_H */
