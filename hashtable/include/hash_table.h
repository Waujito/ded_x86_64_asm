#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

typedef char *ht_key;
typedef int ht_value;

struct hashtable;

struct hashtable *hashtable_ctor();
void hashtable_dtor(struct hashtable *table);

int hashtable_get(struct hashtable *table, ht_key key, ht_value *value);
int hashtable_add(struct hashtable *table, ht_key key, ht_value value);
int hashtable_del(struct hashtable *table, ht_key key);
size_t hashtable_len(struct hashtable *table);

#endif /* HASH_TABLE_H */
