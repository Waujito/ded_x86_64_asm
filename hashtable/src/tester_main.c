#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#include "hash_table.h"

enum ht_op_key {
	HT_ADD,
	HT_DELETE,
	HT_GET
};

struct ht_operation {
	enum ht_op_key op;
	char key[64];
	ht_value value;
};

typedef void (*hashtable_fn)(struct ht_operation *arr, size_t n);

enum tsr_status {
	TSR_SUCCESS=0,
	TSR_SORTED_WRONG=1,
	TSR_INTERNAL=-1,
	TSR_ENOMEM=-2,
	TSR_FOPEN=-3,
};


static double millis_diff (struct timespec end_time, struct timespec start_time) {
	int64_t sec_diff = end_time.tv_sec - start_time.tv_sec;
	int64_t nsec_diff = end_time.tv_nsec - start_time.tv_nsec;
	double millisec_diff = (double)sec_diff * 1000 + (double)nsec_diff / 1000000;

	return millisec_diff;
}

#define START_TIMER(...) \
	struct timespec vtst__start_time = {0};				\
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &vtst__start_time)

#define END_TIMER(...) \
	struct timespec vtst__end_time = {0};				\
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &vtst__end_time)

#define TIME_DIFF(...)	millis_diff(vtst__end_time, vtst__start_time)

struct ht_operation *read_operation_arr(
	const char *filename, size_t len) {

	FILE *arr_file = fopen(filename, "r");
	if (!arr_file) {
		fprintf(stderr, "Cannot open file %s: %s\n", filename, strerror(errno));
		return NULL;
	}

	struct ht_operation *ops = calloc(len, sizeof(struct ht_operation));
	if (!ops) {
		fprintf(stderr, "Allocation error\n");
		fclose(arr_file);
		return NULL;
	}

	for (size_t i = 0; i < len; i++) {
		char operation_key;

		if (fscanf(arr_file, "%c %64s ", &operation_key, ops[i].key) != 2) {
			fprintf(stderr, "fscanf line %zu/%zu \n", i, len);
			fclose(arr_file);
			free(ops);
			return NULL;
		}

		switch (operation_key) {
			case '+':
				ops[i].op = HT_ADD;
				break;
			case 'q':
				ops[i].op = HT_GET;
				break;
			case '-':
				ops[i].op = HT_DELETE;
				break;
			default:
				fprintf(stderr, "invalid key line %zu/%zu \n", i, len);
				fclose(arr_file);
				free(ops);
				return NULL;
		}

		ops[i].value = i;
	}

	fclose(arr_file);

	return ops;
}

#define FILENAME_BUFLEN (128)
enum tsr_status test_one(hashtable_fn ht_fn, size_t len, const char *in_filename, double *time) {
	assert (test_src);
	assert (time);

	struct ht_operation *operations = NULL;

	enum tsr_status ret = TSR_SUCCESS;

	operations = read_operation_arr(in_filename, len);
	if (!operations) {
		ret = TSR_INTERNAL;
		goto exit;
	}

	START_TIMER();
	ht_fn(operations, len);
	END_TIMER();

	double sort_time = TIME_DIFF();
	*time = sort_time;

exit:
	free(operations);

	return ret;
}

void hashtable_test(struct ht_operation *arr, size_t n) {
	struct hashtable *ht = hashtable_ctor();

	for (size_t i = 0; i < n; i++) {
		struct ht_operation op = arr[i];
		switch (op.op) {
			case HT_ADD:
				if (hashtable_add(ht, op.key, op.value)) {
					fprintf(stderr, "Add failed\n");
					abort();
				}
				break;
			case HT_GET:
				if (hashtable_get(ht, op.key, NULL)) {
					fprintf(stderr, "Get failed\n");
					abort();
				};
				break;
			case HT_DELETE:
				hashtable_del(ht, op.key);
				break;
			default:
				fprintf(stderr, "invalid op: %d\n", op.op);
				abort();
		}
	}

	hashtable_dtor(ht);
}

#define TESTSET_LEN (250000 + 10*250000)

int main() {
	double time = 0;
	if (test_one(hashtable_test, TESTSET_LEN, "tests/test_set.in", &time)) {
		fprintf(stderr, "Test failed\n");
		return 1;
	}

	printf("%f\n", time);

	return 0;
}
