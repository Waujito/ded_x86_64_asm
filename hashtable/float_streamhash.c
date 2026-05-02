#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <limits.h>


typedef uint32_t(*uint_hash_fn)(float a, uint32_t mod);

uint32_t int_cast_hash(float a, uint32_t mod) {
	uint32_t casted = (uint32_t)((int)a);
	return casted % mod;
}

static uint32_t hash_bitwise(uint32_t x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x);
	return x;
}

uint32_t bitwise_hash(float a, uint32_t mod) {
	uint32_t casted;
	memcpy(&casted, &a, sizeof(casted));

	return hash_bitwise(casted) % mod;
}

uint32_t mantissa_hash(float a, uint32_t mod) {
	uint32_t casted;
	memcpy(&casted, &a, sizeof(casted));

	uint32_t mantissa = casted & 0x007FFFFF;
	return mantissa % mod;
}

uint32_t exponent_hash(float a, uint32_t mod) {
	uint32_t casted;
	memcpy(&casted, &a, sizeof(casted));

	uint32_t exponent = (casted >> 23) & 0xFF;
	return exponent % mod;
}

uint32_t mantissa_by_exponent_hash(float a, uint32_t mod) {
	return (mantissa_hash(a, mod) * exponent_hash(a, mod)) % mod;
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

	if (!strcmp(dtype_str, "int_cast")) {
		conf->hash_fn = int_cast_hash;
	} else if (!strcmp(dtype_str, "bithash")) {
		conf->hash_fn = bitwise_hash;
	} else if (!strcmp(dtype_str, "mantissa")) {
		conf->hash_fn = mantissa_hash;
	} else if (!strcmp(dtype_str, "exponent")) {
		conf->hash_fn = exponent_hash;
	} else if (!strcmp(dtype_str, "mantissa_by_exponent")) {
		conf->hash_fn = mantissa_by_exponent_hash;
	} else {
		fprintf(stderr, "Unknown hash_type: <%s>\n", dtype_str);
		return 1;
	}

	return 0;
}

int main(int argc, const char *argv[]) {
	struct config conf;
	if (argparse(argc, argv, &conf)) {
		fprintf(stderr, "Usage: %s <mod> hashfn:<int_cast|bithash|mantissa|exponent|mantissa_by_exponent>\n", argv[0]);
		return 1;
	}

	float num;
	while (scanf("%f ", &num) == 1) {
		uint32_t hashed = conf.hash_fn(num, conf.mod);
		printf("%u ", hashed);
	}
	printf("\n");

	return 0;
}
