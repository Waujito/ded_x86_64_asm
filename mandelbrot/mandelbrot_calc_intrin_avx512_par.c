#include <stdint.h>
#include <immintrin.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP

#undef STEP_ARR_LEN
#define STEP_ARR_LEN (16)

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
			__m512d zxs1 = _mm512_setzero_pd();
			__m512d zxs2 = _mm512_setzero_pd();

			__m512d zys1 = _mm512_setzero_pd();
			__m512d zys2 = _mm512_setzero_pd();

			__m512d cys1 = _mm512_set1_pd(cy);
			__m512d cys2 = _mm512_set1_pd(cy);

			__m512d cxs1 = _mm512_set_pd(0, 1., 2., 3., 4., 5., 6., 7.);
			__m512d cxs2 = _mm512_set_pd(8., 9., 10., 11., 12., 13., 14., 15.);

			cxs1 = _mm512_add_pd(cxs1, _mm512_set1_pd((double)x));
			cxs2 = _mm512_add_pd(cxs2, _mm512_set1_pd((double)x));

			cxs1 = _mm512_mul_pd(cxs1, _mm512_set1_pd(2. / width));
			cxs2 = _mm512_mul_pd(cxs2, _mm512_set1_pd(2. / width));

			cxs1 = _mm512_sub_pd(cxs1, _mm512_set1_pd(1.));
			cxs2 = _mm512_sub_pd(cxs2, _mm512_set1_pd(1.));

			cxs1 = _mm512_mul_pd(cxs1, _mm512_set1_pd(state->zoom));
			cxs2 = _mm512_mul_pd(cxs2, _mm512_set1_pd(state->zoom));

			cxs1 = _mm512_sub_pd(cxs1, _mm512_set1_pd(state->shift_left));
			cxs2 = _mm512_sub_pd(cxs2, _mm512_set1_pd(state->shift_left));

			__m512i iters1 = _mm512_setzero_si512();
			__m512i iters2 = _mm512_setzero_si512();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				__m512d zx_dbs1 = _mm512_mul_pd(zxs1, zxs1);
				__m512d zx_dbs2 = _mm512_mul_pd(zxs2, zxs2);
				
				__m512d zy_dbs1 = _mm512_mul_pd(zys1, zys1);
				__m512d zy_dbs2 = _mm512_mul_pd(zys2, zys2);
				
				__m512d zxy_dbs1 = _mm512_mul_pd(zxs1, zys1);
				__m512d zxy_dbs2 = _mm512_mul_pd(zxs2, zys2);

				// * .2
				zxy_dbs1 = _mm512_add_pd(zxy_dbs1, zxy_dbs1);
				zxy_dbs2 = _mm512_add_pd(zxy_dbs2, zxy_dbs2);

				zxs1 = _mm512_sub_pd(zx_dbs1, zy_dbs1);
				zxs2 = _mm512_sub_pd(zx_dbs2, zy_dbs2);

				zxs1 = _mm512_add_pd(zxs1, cxs1);
				zxs2 = _mm512_add_pd(zxs2, cxs2);

				zys1 = _mm512_add_pd(zxy_dbs1, cys1);
				zys2 = _mm512_add_pd(zxy_dbs2, cys2);

				__m512d z_modulos1 = _mm512_add_pd(zx_dbs1, zy_dbs1);
				__m512d z_modulos2 = _mm512_add_pd(zx_dbs2, zy_dbs2);
				__mmask8 compr_results1 = _mm512_cmplt_pd_mask(z_modulos1,
						_mm512_set1_pd(MBT_STOP_RADIUS));
				__mmask8 compr_results2 = _mm512_cmplt_pd_mask(z_modulos2,
						_mm512_set1_pd(MBT_STOP_RADIUS));

				__m512i one512 = _mm512_set1_epi64(1);
				int sm = compr_results1 | compr_results2;

				iters1 = _mm512_mask_add_epi64(iters1, compr_results1, iters1, one512);
				iters2 = _mm512_mask_add_epi64(iters2, compr_results2, iters2, one512);

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int64_t iters_arr[STEP_ARR_LEN];
			_mm512_storeu_si512(((__m512i*)iters_arr), iters2);
			_mm512_storeu_si512(((__m512i*)iters_arr) + 1, iters1);
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
