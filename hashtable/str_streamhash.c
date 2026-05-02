#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

typedef uint32_t(*str_hash_fn)(char *a, uint32_t mod);

uint32_t str_len_hash(char *a, uint32_t mod) {
	return strlen(a) % mod;
}

uint32_t letters_sum_hash(char *a, uint32_t mod) {
	uint32_t sum = 0;
	do {
		sum += *a;
	} while(*(a++) != '\0');

	return sum % mod;
}

#define ALPHABET_STRENGTH (128)

uint32_t poly_hash(char *a, uint32_t mod) {
	uint32_t hash = 0;

	while (*a != '\0') {
		hash = (((hash * ALPHABET_STRENGTH) % mod) + ((uint32_t)(*a) % mod)) % mod;

		a++;
	}

	return hash;
}

unsigned int
xcrc32 (const unsigned char *buf, int len, unsigned int init);

uint32_t crc32_hash(char *a, uint32_t mod) {
	uint32_t hash = xcrc32((unsigned char *)a, strlen(a), 0xffffffff);
	return hash % mod;
}

struct config {
	uint32_t mod;
	str_hash_fn hash_fn;
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

	if (!strcmp(dtype_str, "strlen")) {
		conf->hash_fn = str_len_hash;
	} else if (!strcmp(dtype_str, "letters_sum")) {
		conf->hash_fn = letters_sum_hash;
	} else if (!strcmp(dtype_str, "poly")) {
		conf->hash_fn = poly_hash;
	} else if (!strcmp(dtype_str, "crc32")) {
		conf->hash_fn = crc32_hash;
	} else {
		fprintf(stderr, "Unknown hash_type: <%s>\n", dtype_str);
		return 1;
	}

	return 0;
}

int main(int argc, const char *argv[]) {
	struct config conf;
	if (argparse(argc, argv, &conf)) {
		fprintf(stderr, "Usage: %s <mod> hashfn:<strlen|letters_sum|poly|crc32>\n", argv[0]);
		return 1;
	}

	char str[64];

	while (scanf("%20s", str) == 1) {
		uint32_t hashed = conf.hash_fn(str, conf.mod);
		printf("%u ", hashed);
	}
	printf("\n");

	return 0;
}
