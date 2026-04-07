#include <stdint.h>
#include <immintrin.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP

void calcMandelbrotFloatSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float cr = (x * 2.f - width) / width;
			float ci = (y * 2.f - height) / height;

			cr *= state->zoom;
			ci *= state->zoom;
			cr -= state->shift_left;
			ci -= state->shift_up;

			float zr = 0, zi = 0;

			int iter = 0;
			int niter = 0;
			while (iter < MBT_MAX_ITER && niter < MBT_MAX_ITER) {	
				niter++;

				float next_zr = zr * zr - zi * zi + cr;
				zi = 2.f * zr * zi + ci;
				zr = next_zr;

				if (zr * zr + zi * zi <= MBT_STOP_RADIUS) {
					iter++;
				}
#ifndef MBT_NO_EARLY_STOP
				else break;
#endif
			}

			uint32_t color = (iter == MBT_MAX_ITER) ? 0 : (iter * 0x00000505);

			pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x] = color;
		}
	}
}

void calcMandelbrotDoubleSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double cr = (x * 2. - width) / width;
			double ci = (y * 2. - height) / height;

			cr *= state->zoom;
			ci *= state->zoom;
			cr -= state->shift_left;
			ci -= state->shift_up;

			double zr = 0, zi = 0;

			int iter = 0;
			int niter = 0;
			while (iter < MBT_MAX_ITER && niter < MBT_MAX_ITER) {
				niter++;

				double next_zr = zr * zr - zi * zi + cr;
				zi = 2. * zr * zi + ci;
				zr = next_zr;

				if (zr * zr + zi * zi <= MBT_STOP_RADIUS) {
					iter++;
				}
#ifndef MBT_NO_EARLY_STOP
				else break;
#endif
			}

			uint32_t color = (iter == MBT_MAX_ITER) ? 0 : (iter * 0x00000505);

			pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x] = color;
		}
	}
}
