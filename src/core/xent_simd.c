#include "../xent_internal.h"

#if XENT_ISPC_ENABLED
  #include "xent_ispc_kernels_ispc.h"
  #ifdef __MINGW32__
int _fltused = 0x9875;
  #endif
#endif

static float scalar_sum_f32(float const *values, uint32_t count) {
	float sum = 0.0f;
	for (uint32_t i = 0; i < count; ++i) sum += values [i];
	return sum;
}

static void scalar_fill_f32(float *values, uint32_t count, float value) {
	for (uint32_t i = 0; i < count; ++i) values [i] = value;
}

bool xent_simd_enabled(void) {
#if XENT_ISPC_ENABLED
	return true;
#else
	return false;
#endif
}

float xent_simd_sum_f32(float const *values, uint32_t count) {
	if (!values || count == 0u) return 0.0f;
#if XENT_ISPC_ENABLED
	if (count < 256u) return scalar_sum_f32(values, count);
	return xent_ispc_sum_f32(values, count);
#else
	return scalar_sum_f32(values, count);
#endif
}

void xent_simd_fill_f32(float *values, uint32_t count, float value) {
	if (!values || count == 0u) return;
	scalar_fill_f32(values, count, value);
}
