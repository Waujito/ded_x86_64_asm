#!/bin/bash
set -e

CMD="./build/mandelbrot_bench"

NWARMUP=4
NRUNS=10

run_tests() {
	# Warmup runs
	for i in $(seq $NWARMUP); do
		echo "warmup $i" >&2
		$CMD > /dev/null
	done

	for i in $(seq $NRUNS); do
		echo "test $i" >&2
		$CMD | tail -n 1
	done
}

run_double_intrin_tests() {
	make 1>&2
	run_tests | test_format_name "double_intrin"
}

run_float_intrin_tests() {
	make BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests| test_format_name "float_intrin"
}

run_double_simple_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_simple.c"  1>&2
	run_tests | test_format_name "double_simple"
}

run_float_simple_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_simple.c" BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests | test_format_name "float_simple"
}

test_format_name() {
	sed "s/^/$1,/"
}

HEAD="case,clock,ms"

echo "$HEAD"
#run_double_tests
#run_float_tests

run_double_simple_tests
run_float_simple_tests
