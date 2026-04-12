MANDELBROT_CALC_FILE:=mandelbrot_calc_intrin_avx512.c

build: mandelbrot mandelbrot_bench


run: build
	./build/mandelbrot

run_bench: build
	./build/mandelbrot_bench

mandelbrot: mandelbrot_calc.h $(MANDELBROT_CALC_FILE) mandelbrot.c
	mkdir -p ./build
	gcc $(BENCH_CFLAGS) -O3 -fopenmp -march=native -lSDL3 -lm -ggdb3 $(MANDELBROT_CALC_FILE) mandelbrot.c -o ./build/mandelbrot

mandelbrot_bench: mandelbrot_calc.h $(MANDELBROT_CALC_FILE) mandelbrot_bench.c
	mkdir -p ./build
	gcc $(BENCH_CFLAGS) -DMBT_NO_EARLY_STOP -O3 -march=native -lm -ggdb3 $(MANDELBROT_CALC_FILE) mandelbrot_bench.c -o ./build/mandelbrot_bench

clean:
	rm -rf build
