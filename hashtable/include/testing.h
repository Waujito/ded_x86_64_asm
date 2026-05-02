#ifndef TESTING_H
#define TESTING_H

#include <stddef.h>
#include <errno.h>
#include <stdint.h>

enum ht_op_key {
	HT_ADD,
	HT_DELETE,
	HT_GET
};

struct ht_operation {
	enum ht_op_key op;
	int key;
	int value;
};

typedef void (*hashtable_fn)(struct ht_operation *arr, size_t n);

struct test_sort_params {
	const char *test_src;
	size_t from;
	size_t to;
	size_t step;
	size_t k;
};

enum tsr_status {
	TSR_SUCCESS=0,
	TSR_SORTED_WRONG=1,
	TSR_INTERNAL=-1,
	TSR_ENOMEM=-2,
	TSR_FOPEN=-3,
};

/**
 * Tests the sorting algorithm with a set of tests discribed in tsp.
 *
 * Writes these results to `test_report` file in csv format with header:
 * `test_len,average_time`
 */
enum tsr_status test_hashtable(hashtable_fn ht_fn, struct test_sort_params tsp,
			  const char *test_report);

enum tsr_status test_hashtable_test_set(hashtable_fn ht_fn, const char *dirname, const char *algo_name);

#endif /* TESTING_H */
