build: mandelbrot mandelbrot_bench

run: build
	./build/mandelbrot

run_bench: build
	./build/mandelbrot_bench

mandelbrot: mandelbrot_calc.h mandelbrot_calc.c mandelbrot.c
	mkdir -p ./build
	gcc -O3 -DMBT_PARALLEL_RENDER -fopenmp -march=native -lSDL3 -lm -ggdb3 mandelbrot_calc.c mandelbrot.c -o ./build/mandelbrot

mandelbrot_bench: mandelbrot_calc.h mandelbrot_calc.c mandelbrot_bench.c
	mkdir -p ./build
	gcc $(BENCH_CFLAGS) -DMBT_NO_EARLY_STOP -O3 -march=native -lm -ggdb3 mandelbrot_calc.c mandelbrot_bench.c -o ./build/mandelbrot_bench

clean:
	rm -rf build
