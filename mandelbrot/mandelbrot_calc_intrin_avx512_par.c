#pragma GCC optimize("unroll-loops")
#include <stdint.h>
#include <immintrin.h>
#include <stdio.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP


#ifdef MBT_NCV_BATCH
#define NCV_BATCH MBT_NCV_BATCH
#else
#define NCV_BATCH (2)
#endif


#define STEP_ARR_LEN (8 * NCV_BATCH)

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
			__m512d zxses[NCV_BATCH];
			__m512d zyses[NCV_BATCH];
			__m512d cyses[NCV_BATCH];
			__m512d cxses[NCV_BATCH];
			__m512i iterses[NCV_BATCH];
			__m512d zx_dbses[NCV_BATCH];
			__m512d zy_dbses[NCV_BATCH];
			__m512d zxy_dbses[NCV_BATCH];
			__m512d z_moduloses[NCV_BATCH];
			__mmask8 compr_results[NCV_BATCH];

			for (int i = 0; i < NCV_BATCH; i++) { zxses[i] = _mm512_setzero_pd(); }
			for (int i = 0; i < NCV_BATCH; i++) { zyses[i] = _mm512_setzero_pd(); }

			for (int i = 0; i < NCV_BATCH; i++) { cyses[i] = _mm512_set1_pd(cy); }

			for (int i = 0; i < NCV_BATCH; i++) { 
				cxses[i] = _mm512_set_pd(0 + 8 * i, 1 + 8 * i, 2 + 8 * i,
					3 + 8 * i, 4 + 8 * i, 5 + 8 * i, 6 + 8 * i, 7 + 8 * i);
			}

			for (int i = 0; i < NCV_BATCH; i++)
				cxses[i] = _mm512_add_pd(cxses[i], _mm512_set1_pd((double)x));

			for (int i = 0; i < NCV_BATCH; i++)
				cxses[i] = _mm512_mul_pd(cxses[i], _mm512_set1_pd(2. / width));

			for (int i = 0; i < NCV_BATCH; i++)
				cxses[i] = _mm512_sub_pd(cxses[i], _mm512_set1_pd(1.));

			for (int i = 0; i < NCV_BATCH; i++)
				cxses[i] = _mm512_mul_pd(cxses[i], _mm512_set1_pd(state->zoom));

			for (int i = 0; i < NCV_BATCH; i++)
				cxses[i] = _mm512_sub_pd(cxses[i], _mm512_set1_pd(state->shift_left));

			for (int i = 0; i < NCV_BATCH; i++)
				iterses[i] = _mm512_setzero_si512();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				for (int i = 0; i < NCV_BATCH; i++)
					zx_dbses[i] = _mm512_mul_pd(zxses[i], zxses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					zy_dbses[i] = _mm512_mul_pd(zyses[i], zyses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					zxy_dbses[i] = _mm512_mul_pd(zxses[i], zyses[i]);
				
				// * .2
				for (int i = 0; i < NCV_BATCH; i++)
					zxy_dbses[i] = _mm512_add_pd(zxy_dbses[i], zxy_dbses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					zxses[i] = _mm512_sub_pd(zx_dbses[i], zy_dbses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					zxses[i] = _mm512_add_pd(zxses[i], cxses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					zyses[i] = _mm512_add_pd(zxy_dbses[i], cyses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					z_moduloses[i] = _mm512_add_pd(zx_dbses[i], zy_dbses[i]);

				for (int i = 0; i < NCV_BATCH; i++)
					compr_results[i] = _mm512_cmplt_pd_mask(z_moduloses[i],
						_mm512_set1_pd(MBT_STOP_RADIUS));

				__m512i one512 = _mm512_set1_epi64(1);
				
				int sm = 0;

				for (int i = 0; i < NCV_BATCH; i++) 
					sm |= compr_results[i];

				for (int i = 0; i < NCV_BATCH; i++)
					iterses[i] = _mm512_mask_add_epi64(iterses[i],
						compr_results[i], iterses[i], one512);

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int64_t iters_arr[STEP_ARR_LEN];
			for (int i = 0, j = NCV_BATCH - 1; i < NCV_BATCH; i++, j--) {
				_mm512_storeu_si512(((__m512i*)iters_arr) + i, iterses[j]);
			}
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
