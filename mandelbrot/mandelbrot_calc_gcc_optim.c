#include <stdint.h>
#include <immintrin.h>
#include <stdio.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP

#ifdef MBT_CL_STEP_ARR_LEN
#define STEP_ARR_LEN MBT_CL_STEP_ARR_LEN
#else
#define STEP_ARR_LEN (4)
#endif

void calcMandelbrotFloatSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state) {
#ifdef MBT_PARALLEL_RENDER
	#pragma omp parallel for
#endif
	for (int y = 0; y < height; y++) {
		float cy = (2.f * y) / height - 1;
		cy *= state->zoom / 1.25f;
		cy -= state->shift_up;

		for (int x = 0; x < width; x += STEP_ARR_LEN) {
			float zxs[STEP_ARR_LEN] = {0};
			float zys[STEP_ARR_LEN] = {0};

			float cys[STEP_ARR_LEN] = { 0 };
			for (int i = 0; i < STEP_ARR_LEN; i++) {
				cys[i] = cy;
			}

			float cxs[STEP_ARR_LEN] = { 0 };
			for (int i = 0; i < STEP_ARR_LEN; i++) {
				cxs[i] = x + i;
			}

			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] *= 2.f/width; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] -= 1.f; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] *= state->zoom; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] -= state->shift_left; }

			int iters[STEP_ARR_LEN] = {0};
			for (int i = 0; i < MBT_MAX_ITER; i++) {
				float zx_dbs[STEP_ARR_LEN];
				float zy_dbs[STEP_ARR_LEN];
				float zxy_dbs[STEP_ARR_LEN];
				int incs[STEP_ARR_LEN] = {0};

				for (int i = 0; i < STEP_ARR_LEN; i++) { zx_dbs[i] = zxs[i] * zxs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zy_dbs[i] = zys[i] * zys[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxy_dbs[i] = 2.f * zxs[i] * zys[i]; }
				
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxs[i] = zx_dbs[i] - zy_dbs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxs[i] += cxs[i]; }

				for (int i = 0; i < STEP_ARR_LEN; i++) { zys[i] = zxy_dbs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zys[i] += cys[i]; }

				for (int i = 0; i < STEP_ARR_LEN; i++) { incs[i] = (zx_dbs[i] + zy_dbs[i] <= MBT_STOP_RADIUS); }

				for (int i = 0; i < STEP_ARR_LEN; i++) { iters[i] += incs[i]; }
// 				int sm = 0;
// 				for (int i = 0; i < STEP_ARR_LEN; i++) { sm += incs[i]; }
//
// #ifndef MBT_NO_EARLY_STOP
// 				if (!sm) break;
// #endif
			}

			for (int i = 0; i < STEP_ARR_LEN; i++) {
				int iter = iters[i];
				uint32_t color = (iter == MBT_MAX_ITER) ? 0 : 
					(iter % 43 << 20) + (iter % 91 << 9) + iter;

				pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x + i] = color;
			}

		}
	}
}


void calcMandelbrotDoubleSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state) {
#ifdef MBT_PARALLEL_RENDER
	#pragma omp parallel for
#endif
	for (int y = 0; y < height; y++) {
		double cy = (2. * y) / height - 1;
		cy *= state->zoom / 1.25;
		cy -= state->shift_up;

		for (int x = 0; x < width; x += STEP_ARR_LEN) {
			double zxs[STEP_ARR_LEN] = {0};
			double zys[STEP_ARR_LEN] = {0};

			double cys[STEP_ARR_LEN] = { 0 };
			for (int i = 0; i < STEP_ARR_LEN; i++) {
				cys[i] = cy;
			}

			double cxs[STEP_ARR_LEN] = { 0 };
			for (int i = 0; i < STEP_ARR_LEN; i++) {
				cxs[i] = x + i;
			}

			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] *= 2./width; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] -= 1.; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] *= state->zoom; }
			for (int i = 0; i < STEP_ARR_LEN; i++) { cxs[i] -= state->shift_left; }

			int iters[STEP_ARR_LEN] = {0};
			for (int i = 0; i < MBT_MAX_ITER; i++) {
				double zx_dbs[STEP_ARR_LEN];
				double zy_dbs[STEP_ARR_LEN];
				double zxy_dbs[STEP_ARR_LEN];
				int incs[STEP_ARR_LEN] = {0};

				for (int i = 0; i < STEP_ARR_LEN; i++) { zx_dbs[i] = zxs[i] * zxs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zy_dbs[i] = zys[i] * zys[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxy_dbs[i] = 2. * zxs[i] * zys[i]; }
				
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxs[i] = zx_dbs[i] - zy_dbs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zxs[i] += cxs[i]; }

				for (int i = 0; i < STEP_ARR_LEN; i++) { zys[i] = zxy_dbs[i]; }
				for (int i = 0; i < STEP_ARR_LEN; i++) { zys[i] += cys[i]; }

				for (int i = 0; i < STEP_ARR_LEN; i++) { incs[i] = (zx_dbs[i] + zy_dbs[i] <= MBT_STOP_RADIUS); }

				for (int i = 0; i < STEP_ARR_LEN; i++) { iters[i] += incs[i]; }
				// int sm = 1;

				// for (int i = 0; i < STEP_ARR_LEN; i++) { incs[i] = incs[i] == 0; }
				// for (int i = 0; i < STEP_ARR_LEN; i++) { sm &= incs[i]; }

// #ifndef MBT_NO_EARLY_STOP
				// if (sm) break;
// #endif
			}

			for (int i = 0; i < STEP_ARR_LEN; i++) {
				int iter = iters[i];
				uint32_t color = (iter == MBT_MAX_ITER) ? 0 : 
					(iter % 43 << 20) + (iter % 91 << 9) + iter;

				pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x + i] = color;
			}

		}
	}
}
