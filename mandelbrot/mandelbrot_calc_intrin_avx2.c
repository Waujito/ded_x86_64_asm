#include <stdint.h>
#include <immintrin.h>

#include "mandelbrot_calc.h"

// #define MBT_PARALLEL_RENDER
// #define MBT_NO_EARLY_STOP

#define STEP_ARR_LEN (8)

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
			__m256 zxs = _mm256_setzero_ps();
			__m256 zys = _mm256_setzero_ps();

			__m256 cys = _mm256_set1_ps(cy);

			__m256 cxs = _mm256_set_ps(0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);
			cxs = _mm256_add_ps(cxs, _mm256_set1_ps((float)x));
			cxs = _mm256_mul_ps(cxs, _mm256_set1_ps(2.f / width));
			cxs = _mm256_sub_ps(cxs, _mm256_set1_ps(1.f));
			cxs = _mm256_mul_ps(cxs, _mm256_set1_ps((float)state->zoom));
			cxs = _mm256_sub_ps(cxs, _mm256_set1_ps((float)state->shift_left));

			__m256i iters = _mm256_setzero_si256();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				__m256 zx_dbs = _mm256_mul_ps(zxs, zxs);
				__m256 zy_dbs = _mm256_mul_ps(zys, zys);
				__m256 zxy_dbs = _mm256_mul_ps(zxs, zys);
				// * .2
				zxy_dbs = _mm256_add_ps(zxy_dbs, zxy_dbs);

				zxs = _mm256_sub_ps(zx_dbs, zy_dbs);
				zxs = _mm256_add_ps(zxs, cxs);

				zys = _mm256_add_ps(zxy_dbs, cys);


				__m256 z_modulos = _mm256_add_ps(zx_dbs, zy_dbs);
				__m256 compr_results = _mm256_cmp_ps(z_modulos,
						_mm256_set1_ps(MBT_STOP_RADIUS), _CMP_LE_OQ);

				__m256 one256 = _mm256_castsi256_ps(_mm256_set1_epi32(1));
				volatile int sm = _mm256_movemask_ps(compr_results);
				compr_results = _mm256_and_ps(compr_results, one256);
				iters = _mm256_add_epi32(iters, _mm256_castps_si256(compr_results));

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int32_t iters_arr[STEP_ARR_LEN];
			_mm256_storeu_si256((__m256i*)iters_arr, iters);
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
#define STEP_ARR_LEN (4)

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
			__m256d zxs = _mm256_setzero_pd();
			__m256d zys = _mm256_setzero_pd();

			__m256d cys = _mm256_set1_pd(cy);

			__m256d cxs = _mm256_set_pd(0, 1., 2., 3.);
			cxs = _mm256_add_pd(cxs, _mm256_set1_pd((double)x));
			cxs = _mm256_mul_pd(cxs, _mm256_set1_pd(2. / width));
			cxs = _mm256_sub_pd(cxs, _mm256_set1_pd(1.));
			cxs = _mm256_mul_pd(cxs, _mm256_set1_pd(state->zoom));
			cxs = _mm256_sub_pd(cxs, _mm256_set1_pd(state->shift_left));

			__m256i iters = _mm256_setzero_si256();

			for (int i = 0; i < MBT_MAX_ITER; i++) {
				__m256d zx_dbs = _mm256_mul_pd(zxs, zxs);
				__m256d zy_dbs = _mm256_mul_pd(zys, zys);
				__m256d zxy_dbs = _mm256_mul_pd(zxs, zys);
				// * .2
				zxy_dbs = _mm256_add_pd(zxy_dbs, zxy_dbs);

				zxs = _mm256_sub_pd(zx_dbs, zy_dbs);
				zxs = _mm256_add_pd(zxs, cxs);

				zys = _mm256_add_pd(zxy_dbs, cys);


				__m256d z_modulos = _mm256_add_pd(zx_dbs, zy_dbs);
				__m256d compr_results = _mm256_cmp_pd(z_modulos,
						_mm256_set1_pd(MBT_STOP_RADIUS), _CMP_LE_OQ);

				__m256d one256 = _mm256_castsi256_pd(_mm256_set1_epi64x(1));
				volatile int sm = _mm256_movemask_pd(compr_results);
				compr_results = _mm256_and_pd(compr_results, one256);
				iters = _mm256_add_epi64(iters, _mm256_castpd_si256(compr_results));

#ifndef MBT_NO_EARLY_STOP
				if (!sm) break;
#endif
			}

			int64_t iters_arr[STEP_ARR_LEN];
			_mm256_storeu_si256((__m256i*)iters_arr, iters);
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
