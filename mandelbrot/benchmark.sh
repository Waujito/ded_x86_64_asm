#!/bin/bash
set -e

CMD="./build/mandelbrot_bench"

NWARMUP=4
NRUNS=10

run_tests() {
	# Warmup runs
	for i in $(seq $NWARMUP); do
		$CMD > /dev/null
	done

	for i in $(seq $NRUNS); do
		$CMD | tail -n 1
	done
}

run_double_tests() {
	make 1>&2
	run_tests
}

run_float_tests() {
	make BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests
}

test_format_name() {
	sed "s/^/$1,/"
}

HEAD="case,clock,ms"

echo "$HEAD"
run_double_tests | test_format_name "double_intrin"
run_float_tests  | test_format_name "float_intrin"
