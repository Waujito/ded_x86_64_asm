#!/bin/bash
set -e

CMD="./build/mandelbrot_bench"

NWARMUP=4
NRUNS=10


test_format_name() {
	sed "s/^/$1,/"
}

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

run_double_intrin_avx2_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_intrin_avx2.c" 1>&2
	run_tests | test_format_name "double_intrin_avx2"
}

run_float_intrin_avx2_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_intrin_avx2.c" BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests| test_format_name "float_intrin_avx2"
}

run_double_intrin_avx512_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_intrin_avx512.c" 1>&2
	run_tests | test_format_name "double_intrin_avx512"
}

run_double_intrin_avx512_par_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_intrin_avx512_par.c" 1>&2
	run_tests | test_format_name "double_intrin_avx512_par"
}

run_float_intrin_avx512_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_intrin_avx512.c" BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests| test_format_name "float_intrin_avx512"
}

run_double_simple_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_simple.c"  1>&2
	run_tests | test_format_name "double_simple"
}

run_float_simple_tests() {
	make MANDELBROT_CALC_FILE="mandelbrot_calc_simple.c" BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS" 1>&2
	run_tests | test_format_name "float_simple"
}

OPTIM_LIM=64

run_double_gcc_optim_tests() {
	for ((i=1; i <= OPTIM_LIM; i*=2)); do
		make MANDELBROT_CALC_FILE="mandelbrot_calc_gcc_optim.c" BENCH_CFLAGS="-DMBT_CL_STEP_ARR_LEN=$i" 1>&2
		run_tests | test_format_name "double_gcc_optim_$i"
	done
}

run_float_gcc_optim_tests() {
	for ((i=1; i <= OPTIM_LIM; i*=2)); do
		make MANDELBROT_CALC_FILE="mandelbrot_calc_gcc_optim.c" BENCH_CFLAGS="-DMBT_MANDELBROT_FLOATS -DMBT_CL_STEP_ARR_LEN=$i" 1>&2
		run_tests | test_format_name "float_gcc_optim_$i"
	done
}

HEAD="case,clock,ms"

echo "$HEAD"

run_double_intrin_avx2_tests
run_float_intrin_avx2_tests

run_double_intrin_avx512_tests
run_float_intrin_avx512_tests

run_double_simple_tests
run_float_simple_tests

run_double_gcc_optim_tests
run_float_gcc_optim_tests

run_double_intrin_avx512_par_tests
