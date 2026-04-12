#include <stdint.h>
#include <immintrin.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP

#define STEP_ARR_LEN (16)

void calcMandelbrotFloatSet(uint32_t *pixels_ptr, int pitch, int height, int width,
			    struct MandelbrotState *state) {
#ifdef MBT_PARALLEL_RENDER
	#pragma omp parallel for
#endif
	for (int y = 0; y < height; y++) {
		float cy = (2. * y) / height - 1;
		cy *= state->zoom / 1.25;
		cy -= state->shift_up;

		for (int x = 0; x < width; x += STEP_ARR_LEN) {
			__m512 zxs = _mm512_setzero_ps();
			__m512 zys = _mm512_setzero_ps();

			__m512 cys = _mm512_set1_ps(cy);

			__m512 cxs = _mm512_set_ps(0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
			cxs = _mm512_add_ps(cxs, _mm512_set1_ps((float)x));
			cxs = _mm512_mul_ps(cxs, _mm512_set1_ps(2.f / width));
			cxs = _mm512_sub_ps(cxs, _mm512_set1_ps(1.f));
			cxs = _mm512_mul_ps(cxs, _mm512_set1_ps((float)state->zoom));
			cxs = _mm512_sub_ps(cxs, _mm512_set1_ps((float)state->shift_left));

			__m512i iters = _mm512_setzero_si512();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				__m512 zx_dbs = _mm512_mul_ps(zxs, zxs);
				__m512 zy_dbs = _mm512_mul_ps(zys, zys);
				__m512 zxy_dbs = _mm512_mul_ps(zxs, zys);
				// * .2
				zxy_dbs = _mm512_add_ps(zxy_dbs, zxy_dbs);

				zxs = _mm512_sub_ps(zx_dbs, zy_dbs);
				zxs = _mm512_add_ps(zxs, cxs);

				zys = _mm512_add_ps(zxy_dbs, cys);


				__m512 z_modulos = _mm512_add_ps(zx_dbs, zy_dbs);
				__mmask16 compr_results = _mm512_cmplt_ps_mask(z_modulos,
						_mm512_set1_ps(MBT_STOP_RADIUS));

				__m512i one512 = _mm512_set1_epi32(1);
				int sm = compr_results;

				iters = _mm512_mask_add_epi32(iters, compr_results, iters, one512);

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int32_t iters_arr[STEP_ARR_LEN];
			_mm512_storeu_si512((__m512i*)iters_arr, iters);
			// We should go in reverse because of little-endianness
			int j = STEP_ARR_LEN - 1;

			for (int i = 0; i < STEP_ARR_LEN; i++, j--) {
				int iter = iters_arr[j];
				uint32_t color = (iter == MBT_MAX_ITER) ? 0 : 
					(iter % 43 << 20) + (iter % 91 << 9) + iter;

				pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x + i] = color;
			}

		}
	}
}

#undef STEP_ARR_LEN
#define STEP_ARR_LEN (8)

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
			__m512d zxs = _mm512_setzero_pd();
			__m512d zys = _mm512_setzero_pd();

			__m512d cys = _mm512_set1_pd(cy);

			__m512d cxs = _mm512_set_pd(0, 1., 2., 3., 4., 5., 6., 7.);
			cxs = _mm512_add_pd(cxs, _mm512_set1_pd((double)x));
			cxs = _mm512_mul_pd(cxs, _mm512_set1_pd(2. / width));
			cxs = _mm512_sub_pd(cxs, _mm512_set1_pd(1.));
			cxs = _mm512_mul_pd(cxs, _mm512_set1_pd(state->zoom));
			cxs = _mm512_sub_pd(cxs, _mm512_set1_pd(state->shift_left));

			__m512i iters = _mm512_setzero_si512();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				__m512d zx_dbs = _mm512_mul_pd(zxs, zxs);
				__m512d zy_dbs = _mm512_mul_pd(zys, zys);
				__m512d zxy_dbs = _mm512_mul_pd(zxs, zys);
				// * .2
				zxy_dbs = _mm512_add_pd(zxy_dbs, zxy_dbs);

				zxs = _mm512_sub_pd(zx_dbs, zy_dbs);
				zxs = _mm512_add_pd(zxs, cxs);

				zys = _mm512_add_pd(zxy_dbs, cys);


				__m512d z_modulos = _mm512_add_pd(zx_dbs, zy_dbs);
				__mmask8 compr_results = _mm512_cmplt_pd_mask(z_modulos,
						_mm512_set1_pd(MBT_STOP_RADIUS));

				__m512i one512 = _mm512_set1_epi64(1);
				int sm = compr_results;

				iters = _mm512_mask_add_epi64(iters, compr_results, iters, one512);

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int64_t iters_arr[STEP_ARR_LEN];
			_mm512_storeu_si512((__m512i*)iters_arr, iters);
			// We should go in reverse because of little-endianness
			int j = STEP_ARR_LEN - 1;

			for (int i = 0; i < STEP_ARR_LEN; i++, j--) {
				int iter = iters_arr[j];
				uint32_t color = (iter == MBT_MAX_ITER) ? 0 : 
					(iter % 43 << 20) + (iter % 91 << 9) + iter;

				pixels_ptr[y * (pitch / sizeof(*pixels_ptr)) + x + i] = color;
			}

		}
	}	
}
