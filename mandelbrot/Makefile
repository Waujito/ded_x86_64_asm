build: mandelbrot

run: build
	./build/mandelbrot

mandelbrot: mandelbrot.c
	mkdir -p ./build
	gcc -Og -lSDL3 -lm -ggdb3 mandelbrot.c -o ./build/mandelbrot

clean:
	rm -rf build/mandelbrot
