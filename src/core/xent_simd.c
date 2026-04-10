#include "../xent_internal.h"

#if XENT_HIGHWAY_ENABLED
bool xent_highway_probe(void);
float xent_highway_sum_f32(const float *values, uint32_t count);
void xent_highway_fill_f32(float *values, uint32_t count, float value);
#endif

static float xent_scalar_sum_f32(const float *values, uint32_t count) {
    float sum = 0.0f;
    for (uint32_t i = 0; i < count; ++i) {
        sum += values[i];
    }
    return sum;
}

static void xent_scalar_fill_f32(float *values, uint32_t count, float value) {
    for (uint32_t i = 0; i < count; ++i) {
        values[i] = value;
    }
}

bool xent_is_highway_enabled(void) {
#if XENT_HIGHWAY_ENABLED
    return xent_highway_probe();
#else
    return false;
#endif
}

float xent_simd_sum_f32(const float *values, uint32_t count) {
    if (!values || count == 0u) {
        return 0.0f;
    }
#if XENT_HIGHWAY_ENABLED
    /* For medium arrays, compiler auto-vectorized scalar loop is often faster than dispatch overhead. */
    if (count < 32768u) {
        return xent_scalar_sum_f32(values, count);
    }
    return xent_highway_sum_f32(values, count);
#else
    return xent_scalar_sum_f32(values, count);
#endif
}

void xent_simd_fill_f32(float *values, uint32_t count, float value) {
    if (!values || count == 0u) {
        return;
    }
#if XENT_HIGHWAY_ENABLED
    /* Keep tiny/medium fills on scalar path; reserve Highway for large batches. */
    if (count < 32768u) {
        xent_scalar_fill_f32(values, count, value);
        return;
    }
    xent_highway_fill_f32(values, count, value);
#else
    xent_scalar_fill_f32(values, count, value);
#endif
}
