build: mandelbrot

run: build
	./build/mandelbrot

mandelbrot: mandelbrot.c
	mkdir -p ./build
	gcc -O2 -lSDL3 -lm mandelbrot.c -o ./build/mandelbrot

clean:
	rm -rf build/mandelbrot
