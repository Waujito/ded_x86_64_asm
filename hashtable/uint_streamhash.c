#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <limits.h>


typedef uint32_t(*uint_hash_fn)(uint32_t a, uint32_t mod);

uint32_t raw_hash(uint32_t a, uint32_t mod) {
	return a % mod;
}

static uint32_t hash_bitwise(uint32_t x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x);
	return x;
}

uint32_t bitwise_hash(uint32_t a, uint32_t mod) {
	return hash_bitwise(a) % mod;
}

#define KNUTH_A 0x9E3779B9  // floor(2^32 * (sqrt(5)-1)/2)

uint32_t knuth_const_hash(uint32_t a, uint32_t mod) {
	uint64_t product = (uint64_t)a * KNUTH_A;
	uint32_t fractional = product & 0xFFFFFFFF;
	return fractional % mod;
}

struct config {
	uint32_t mod;
	uint_hash_fn hash_fn;
};

static int argparse(int argc, const char *argv[], struct config *conf) {
	if (argc != 3) {
		return 1;
	}

	char *mod_end;
	unsigned long mod = strtoul(argv[1], &mod_end, 10);
	if (errno || *argv[1] == '\0' || *mod_end != '\0') {
		return 1;
	}

	conf->mod = (uint32_t)mod;

	const char *dtype_str = argv[2];

	if (!strcmp(dtype_str, "raw_mod")) {
		conf->hash_fn = raw_hash;
	} else if (!strcmp(dtype_str, "bithash")) {
		conf->hash_fn = bitwise_hash;
	} else if (!strcmp(dtype_str, "knuth_const")) {
		conf->hash_fn = knuth_const_hash;
	} else {
		fprintf(stderr, "Unknown hash_type: <%s>\n", dtype_str);
		return 1;
	}

	return 0;
}

int main(int argc, const char *argv[]) {
	struct config conf;
	if (argparse(argc, argv, &conf)) {
		fprintf(stderr, "Usage: %s <mod> hashfn:<raw_mod|bithash|knuth_const>\n", argv[0]);
		return 1;
	}

	uint32_t num;
	while (scanf("%u", &num) == 1) {
		uint32_t hashed = conf.hash_fn(num, conf.mod);
		printf("%u ", hashed);
	}
	printf("\n");

	return 0;
}
