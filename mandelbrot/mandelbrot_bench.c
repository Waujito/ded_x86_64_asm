#define _GNU_SOURCE
#include <sched.h>
#include <x86intrin.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "mandelbrot_calc.h"

static double millis_diff (struct timespec end_time, struct timespec start_time) {
	int64_t sec_diff = end_time.tv_sec - start_time.tv_sec;
	int64_t nsec_diff = end_time.tv_nsec - start_time.tv_nsec;
	double millisec_diff = (double)sec_diff * 1000 + (double)nsec_diff / 1000000;

	return millisec_diff;
}

#define START_TIMER(...) \
        struct timespec vtst__start_time = {0};                         \
        timespec_get(&vtst__start_time, TIME_UTC)

#define END_TIMER(...) \
        struct timespec vtst__end_time = {0};                           \
        timespec_get(&vtst__end_time, TIME_UTC)

#define TIME_DIFF(...)  millis_diff(vtst__end_time, vtst__start_time)

static int pin_cpu(int cpu_id) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(3, &mask); // Pin to Core 3
	return sched_setaffinity(0, sizeof(mask), &mask);
}

#define PIN_CPU_ID (3)

#define WIDTH  (800)
#define HEIGHT (600)
#define PITCH WIDTH

uint32_t pixels[WIDTH * HEIGHT];

int main() {
	pin_cpu(PIN_CPU_ID);

	struct MandelbrotState state = {
		.shift_left = 0.,
		.shift_up = 0.,
		.zoom = 1.
	};

	unsigned int rdt_id1 = 0;
	unsigned int rdt_id2 = 0;

	START_TIMER();
	unsigned long long startClocks = __rdtscp(&rdt_id1);	

	calcMandelbrotSet(pixels, PITCH, HEIGHT, WIDTH, &state);

	unsigned long long endClocks = __rdtscp(&rdt_id2);
	END_TIMER();

	if (rdt_id1 != rdt_id2) {
		fprintf(stderr, "Assertion failed: proc migrated core\n");
		abort();
	}

	printf("clocks,ms\n");
	printf("%llu,%f\n", endClocks - startClocks, TIME_DIFF());

	return 0;
}
