#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <nmmintrin.h>

#include "hash_table.h"
#include "list.h"

size_t my_strlen(char *key);

// #define USE_HT_MY_STRLEN
static inline size_t strlen_fun(char *key) {
#ifdef USE_HT_MY_STRLEN
	return my_strlen(key);
#else
	return strlen(key);
#endif
}

struct hashtable {
	struct list *arr;
	size_t arr_len;
	size_t n_elements;
};

#define GET_KEY_PTR(hashed_element) (hashed_element.keylen > 64 ? hashed_element.key_ptr : hashed_element.key_arr)

#define STARTING_ARRLEN (1024)
#define CRITICAL_LOAD_FACTOR (10.)

static int is_keys_equal(ht_key key1, ht_key key2, size_t klen);
static int probe_load_factor(struct hashtable *table);

static inline list_ptr_t bucket_find_node(struct list *bucket, ht_key key) {
	assert(bucket);
	assert(bucket->array);

	for (list_ptr_t node = bucket->array[0].next;
		node != LIST_ROOT_EL;
		node = bucket->array[node].next) {
		if (is_keys_equal(GET_KEY_PTR(bucket->array[node].value), key,
			bucket->array[node].value.keylen)) {
			return node;
		}
	}

	return LIST_ROOT_EL;
}

static DSError_t bucket_init(struct list *bucket) {
	DSError_t ret = list_ctor(bucket);
	if (ret) {
		return ret;
	}

	return list_set_capacity(bucket, 20);
}

static void bucket_cleanup(struct list *bucket) {
	if (!bucket || !bucket->array) {
		return;
	}

	for (list_ptr_t node = bucket->array[0].next;
		node != LIST_ROOT_EL;
		node = bucket->array[node].next) {
		if (bucket->array[node].value.keylen > 64) {
			free(bucket->array[node].value.key_ptr);
			bucket->array[node].value.key_ptr = NULL;
		}
	}

	list_dtor(bucket);
}

struct hashtable *hashtable_ctor() {
	struct hashtable *hst = calloc(1, sizeof(struct hashtable));

	if (!hst)
		return NULL;

	hst->arr = calloc(STARTING_ARRLEN, sizeof(struct list));
	if (!hst->arr) {
		free(hst);
		return NULL;
	}

	hst->arr_len = STARTING_ARRLEN;
	hst->n_elements = 0;

	for (size_t i = 0; i < hst->arr_len; i++) {
		if (bucket_init(&hst->arr[i]) != DS_OK) {
			for (size_t j = 0; j < i; j++) {
				bucket_cleanup(&hst->arr[j]);
			}

			free(hst->arr);
			free(hst);
			return NULL;
		}
	}

	return hst;
}

void hashtable_dtor(struct hashtable *table) {
	for (size_t i = 0; i < table->arr_len; i++) {
		bucket_cleanup(&table->arr[i]);
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

static int bucket_move_all(struct hashtable *table, struct list *new_arr,
				 size_t new_len) {
	for (size_t i = 0; i < table->arr_len; i++) {
		struct list *bucket = &table->arr[i];

		for (list_ptr_t node = bucket->array[0].next;
			node != LIST_ROOT_EL;
			node = bucket->array[node].next) {
			list_dtype value = bucket->array[node].value;
			uint32_t new_hash = hash_key(GET_KEY_PTR(value), new_len);

			if (list_insert(&new_arr[new_hash], LIST_ROOT_EL, value, NULL) != DS_OK) {
				return 1;
			}
		}
	}

	return 0;
}

int hashtable_add(struct hashtable *table, ht_key key, ht_value value) {
	uint32_t hash = hash_key(key, table->arr_len);	

	struct list *bucket = &table->arr[hash];
	list_ptr_t node = bucket_find_node(bucket, key);

	if (node != LIST_ROOT_EL) {
		bucket->array[node].value.value = value;
		return 0;
	}
	

	list_dtype item = {
		.value = value,
		.keylen = strlen_fun(key) + 1,
	};

	if (item.keylen > 64) {
		item.key_ptr = strdup(key);
		if (!item.key_ptr) {
			return 1;
		}
	} else {
		memcpy(item.key_arr, key, item.keylen);
	}

	if (list_insert_ptr(bucket, LIST_ROOT_EL, &item, NULL) != DS_OK) {
		if (item.keylen > 64) {
			free(item.key_ptr);
		}
		return 1;
	}

	table->n_elements++;
	probe_load_factor(table);

	return 0;
}

int rehash(struct hashtable *table, size_t new_cap) {
	struct list *new_arr = calloc(new_cap, sizeof(struct list));
	if (!new_arr) {
		return 1;
	}

	for (size_t i = 0; i < new_cap; i++) {
		if (bucket_init(&new_arr[i]) != DS_OK) {
			for (size_t j = 0; j < i; j++) {
				bucket_cleanup(&new_arr[j]);
			}
			free(new_arr);
			return 1;
		}
	}

	if (bucket_move_all(table, new_arr, new_cap) != 0) {
		for (size_t i = 0; i < new_cap; i++) {
			bucket_cleanup(&new_arr[i]);
		}
		free(new_arr);
		return 1;
	}

	for (size_t i = 0; i < table->arr_len; i++) {
		list_dtor(&table->arr[i]);
	}

	free(table->arr);
	table->arr = new_arr;
	table->arr_len = new_cap;
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
	struct list *bucket = &table->arr[hash];
	list_ptr_t node = bucket_find_node(bucket, key);

	if (node == LIST_ROOT_EL) {
		return 1;
	}

	if (value) {
		*value = bucket->array[node].value.value;
	}

	return 0;
}

int hashtable_del(struct hashtable *table, ht_key key) {
	uint32_t hash = hash_key(key, table->arr_len);
	struct list *bucket = &table->arr[hash];
	list_ptr_t node = bucket_find_node(bucket, key);

	if (node == LIST_ROOT_EL) {
		return 1;
	}

	if (bucket->array[node].value.keylen > 64) {
		free(bucket->array[node].value.key_ptr);
	}
	if (list_drop(bucket, node) != DS_OK) {
		return 1;
	}

	table->n_elements--;
	return 0;
}

size_t hashtable_len(struct hashtable *ht) {
	return ht->n_elements;
}
