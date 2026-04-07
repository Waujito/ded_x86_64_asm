#ifndef MANDELBROT_CALC_H
#define MANDELBROT_CALC_H

#include <stdint.h>

struct MandelbrotState {
	double shift_left;
	double shift_up;
	double zoom;
};

// #define MBT_MANDELBROT_FLOATS

#define MBT_MAX_ITER (255)
#define MBT_STOP_RADIUS (10.)

void calcMandelbrotDoubleSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state);

void calcMandelbrotFloatSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state);

#ifdef MBT_MANDELBROT_FLOATS
#define calcMandelbrotSet calcMandelbrotFloatSet
#else
#define calcMandelbrotSet calcMandelbrotDoubleSet
#endif

#endif /* MANDELBROT_CALC_H */
