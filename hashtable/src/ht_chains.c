#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <nmmintrin.h>

#include "hash_table.h"

size_t my_strlen(char *key);

// #define USE_HT_MY_STRLEN
static inline size_t strlen_fun(char *key) {
#ifdef USE_HT_MY_STRLEN
	return my_strlen(key);
#else
	return strlen(key);
#endif
}

struct hashed_element {
	ht_key key;
	ht_value value;
	size_t keylen;

	struct hashed_element *next;
};

struct hashtable {
	struct hashed_element **arr;
	size_t arr_len;
	size_t n_elements;
};

#define STARTING_ARRLEN (1024)
#define CRITICAL_LOAD_FACTOR (10.)

struct hashtable *hashtable_ctor() {
	struct hashtable *hst = calloc(1, sizeof(struct hashtable));

	if (!hst) 
		return NULL;

	hst->arr = calloc(STARTING_ARRLEN, sizeof(struct hashed_element *));
	if (!hst->arr) {
		free(hst);
		return NULL;
	}
	hst->arr_len = STARTING_ARRLEN;
	hst->n_elements = 0;

	return hst;
}

void hashtable_del_unit(struct hashed_element *el) {
	if (el->next) {
		hashtable_del_unit(el->next);
	}

	free(el);
}

void hashtable_dtor(struct hashtable *table) {
	for (size_t i = 0; i < table->arr_len; i++) {
		if (table->arr[i]) {
			hashtable_del_unit(table->arr[i]);
		}
	}

	free(table->arr);
	table->arr = NULL;
	table->arr_len = 0;
	table->n_elements = 0;

	free(table);
}

#define CRC32_INIT (0xffffffff)
#ifdef CRC32_INTRIN
__attribute__((noinline))
static uint32_t hash_key(ht_key key, uint32_t modulo) {
	unsigned int crc = CRC32_INIT;

	for (size_t i = 0; key[i] != '\0'; i++) {
		crc = _mm_crc32_u8(crc, (unsigned char)key[i]);
	}

	return crc % modulo;
}
#else
unsigned int
xcrc32 (const unsigned char *buf, int len, unsigned int init);

__attribute__((noinline))
uint32_t hash_key(ht_key key, uint32_t modulo) {
	return xcrc32((unsigned char *)key, strlen_fun(key), CRC32_INIT) % modulo;
}
#endif

#ifdef IS_K_EQ_ASM

static inline unsigned long long check_eq_strings_64(char *key1, char *key2) {
	unsigned long long mask;

	asm volatile (
		"\n\t .intel_syntax noprefix"

		"\n\t vmovdqu8 zmm0, [%[key1]]"
		"\n\t vmovdqu8 zmm1, [%[key2]]"
		"\n\t vpcmpeqb k1, zmm0, zmm1"
		"\n\t kmovq %[mask], k1"

		"\n\t .att_syntax"
		"\n\t"
		: [mask] "=r" (mask)
		: [key1] "r" (key1),
		  [key2] "r" (key2)
		: "zmm0", "zmm1", "k1", "memory"
	);

	return mask;
}
static inline int strcmp_equal(ht_key key1, ht_key key2) {
	return !strcmp(key1, key2);
}

static int is_keys_equal(ht_key key1, ht_key key2, size_t klen) {
	if (klen > 64) return strcmp_equal(key1, key2);

	int64_t eq_mask = check_eq_strings_64(key1, key2);
	int64_t mask = -1LL;

	if (klen != 64) {
		mask = (1LL << klen) - 1LL;
	}

	if ((~eq_mask) & mask) return 0;
	else return 1;
}
#else
static int is_keys_equal(ht_key key1, ht_key key2, size_t klen) {
	return !strncmp(key1, key2, klen);
}
#endif

struct hashed_element **find_bucket_target(struct hashed_element **entry,
					   ht_key key) {
	struct hashed_element *htel = *entry;
	while (htel != NULL) {
		if (is_keys_equal(htel->key, key, htel->keylen)) {
			return entry;
		}

		entry = &htel->next;
		htel = htel->next;
	}

	return entry;
}

int probe_load_factor(struct hashtable *table);

int hashtable_add(struct hashtable *table, ht_key key, ht_value value) {
	uint32_t hash = hash_key(key, table->arr_len);	
	size_t keylen = strlen_fun(key);

	struct hashed_element **destination_el = find_bucket_target(&table->arr[hash], key);

	if (*destination_el) {
		(*destination_el)->value = value;
		return 0;
	}

	struct hashed_element *new_element = calloc(1, sizeof(struct hashed_element));
	if (!new_element) {
		return 1;
	}

	new_element->key = strdup(key);
	new_element->keylen = keylen;
	if (!new_element->key) {
		free(new_element);
		return 1;
	}
	new_element->value = value;
	new_element->next = NULL;

	*destination_el = new_element;
	table->n_elements++;

	probe_load_factor(table);

	return 0;
}

int rehash(struct hashtable *table, size_t new_cap) {
	struct hashed_element **new_arr = calloc(new_cap,
					sizeof(struct hashed_element *));

	if (!new_arr) {
		return 1;
	}


	size_t old_arr_len = table->arr_len;
	struct hashed_element **old_arr = table->arr;

	for (size_t i = 0; i < old_arr_len; i++) {
		struct hashed_element *element = old_arr[i];

		while (element) {
			uint32_t new_hash = hash_key(element->key, new_cap);
			struct hashed_element **destination_el = find_bucket_target(
						&new_arr[new_hash], element->key);

			(*destination_el) = element;
			struct hashed_element *next_element = element->next;
			element->next = NULL;

			element = next_element;
		}
	}

	free(table->arr);
	table->arr_len = new_cap;
	table->arr = new_arr;

	return 0;
}

int probe_load_factor(struct hashtable *table) {
	double load_factor = (double)table->n_elements / table->arr_len;

	if (load_factor < CRITICAL_LOAD_FACTOR) {
		return 0;
	}

	return rehash(table, table->arr_len * 2);
}

int hashtable_get(struct hashtable *table, ht_key key, ht_value *value) {
	uint32_t hash = hash_key(key, table->arr_len);

	struct hashed_element *element = table->arr[hash];

	while (element) {
		if (is_keys_equal(element->key, key, element->keylen)) {
			if (value) {
				memcpy(value, &element->value, sizeof(*value));
			}
			return 0;
		}

		element = element->next;
	}

	return 1;
}

int hashtable_del(struct hashtable *table, ht_key key) {
	uint32_t hash = hash_key(key, table->arr_len);

	struct hashed_element **elptr = &table->arr[hash];

	struct hashed_element *htel = *elptr;
	while (htel != NULL) {
		if (is_keys_equal(htel->key, key, htel->keylen)) {
			*elptr = htel->next;
			return 0;
		}

		elptr = &htel->next;
		htel = htel->next;
	}

	return 1;
}

size_t hashtable_len(struct hashtable *ht) {
	return ht->n_elements;
}
