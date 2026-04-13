// ISPC SIMD kernels for xent-core
// Multi-target dispatch: ISPC generates SSE4 + AVX2 variants and picks at runtime.

// ─── existing: float array sum reduction ───
export uniform float xent_ispc_sum_f32(const uniform float values[],
                                       uniform unsigned int32 count) {
    float partial = 0.0f;
    foreach (i = 0 ... count) {
        partial += values[i];
    }
    return reduce_add(partial);
}

export uniform bool xent_ispc_probe() {
    return true;
}

// ─── A: batch pixel quantize ───
// Applies roundf(v * scale) / scale to each element.
export void xent_ispc_quantize_f32(uniform float values[],
                                    uniform unsigned int32 count,
                                    uniform float scale) {
    uniform float inv_scale = 1.0f / scale;
    foreach (i = 0 ... count) {
        values[i] = round(values[i] * scale) * inv_scale;
    }
}

// ─── B: invalidate_all_layout — masked OR + masked zero ───
// For each i where mask[i] != 0: flags[i] |= or_value
export void xent_ispc_or_u32_masked(uniform unsigned int32 flags[],
                                     const uniform unsigned int8 mask[],
                                     uniform unsigned int32 count,
                                     uniform unsigned int32 or_value) {
    foreach (i = 0 ... count) {
        if (mask[i] != 0) {
            flags[i] = flags[i] | or_value;
        }
    }
}

// For each i where mask[i] != 0: out[i] = 0
export void xent_ispc_zero_u8_masked(uniform unsigned int8 out[],
                                      const uniform unsigned int8 mask[],
                                      uniform unsigned int32 count) {
    foreach (i = 0 ... count) {
        if (mask[i] != 0) {
            out[i] = 0;
        }
    }
}

// ─── C: clear dirty via index list (scatter-zero) ───
// For each index in indices[0..count-1]:  flags[indices[j]] = 0
export void xent_ispc_scatter_zero_u32(uniform unsigned int32 flags[],
                                        const uniform unsigned int32 indices[],
                                        uniform unsigned int32 count) {
    foreach (i = 0 ... count) {
        unsigned int32 idx = indices[i];
        flags[idx] = 0;
    }
}

// ─── D: flex line stats — multi-reduction ───
// Given SoA arrays extracted from FlexChildData, computes:
//   sum_outer_main = sum(base_main[i] + margin_lead[i] + margin_trail[i])
//   sum_grow       = sum(grow[i])
//   sum_shrink_w   = sum(shrink[i] * select(base_main[i] > 0, base_main[i], 1))
//   max_cross_outer= max(base_cross[i] + margin_cross_lead[i] + margin_cross_trail[i])
// gap contribution is added by the caller: sum_outer_main += gap * (count - 1)
export void xent_ispc_flex_line_stats(
    const uniform float base_main[],
    const uniform float margin_lead[],
    const uniform float margin_trail[],
    const uniform float base_cross[],
    const uniform float margin_cross_lead[],
    const uniform float margin_cross_trail[],
    const uniform float grow[],
    const uniform float shrink[],
    uniform unsigned int32 count,
    uniform float *uniform out_sum_outer_main,
    uniform float *uniform out_sum_grow,
    uniform float *uniform out_sum_shrink_weight,
    uniform float *uniform out_max_cross_outer)
{
    float p_outer = 0.0f;
    float p_grow  = 0.0f;
    float p_sw    = 0.0f;
    float p_cross = 0.0f;
    foreach (i = 0 ... count) {
        p_outer += base_main[i] + margin_lead[i] + margin_trail[i];
        p_grow  += grow[i];
        float bm = base_main[i];
        p_sw    += shrink[i] * select(bm > 0.0f, bm, 1.0f);
        p_cross  = max(p_cross, base_cross[i] + margin_cross_lead[i] + margin_cross_trail[i]);
    }
    *out_sum_outer_main    = reduce_add(p_outer);
    *out_sum_grow          = reduce_add(p_grow);
    *out_sum_shrink_weight = reduce_add(p_sw);
    *out_max_cross_outer   = reduce_max(p_cross);
}

// ─── E: flex grow distribution ───
// final_main[i] = base_main[i] + delta * (grow[i] / sum_grow)
export void xent_ispc_flex_distribute_grow(
    const uniform float base_main[],
    const uniform float grow_arr[],
    uniform float final_main[],
    uniform unsigned int32 count,
    uniform float delta,
    uniform float sum_grow)
{
    uniform float inv = delta / sum_grow;
    foreach (i = 0 ... count) {
        final_main[i] = base_main[i] + grow_arr[i] * inv;
    }
}

// ─── E: flex shrink distribution ───
// weight[i] = shrink[i] * max(base_main[i], 1)
// final_main[i] = max(0, base_main[i] + delta * (weight[i] / sum_shrink_weight))
export void xent_ispc_flex_distribute_shrink(
    const uniform float base_main[],
    const uniform float shrink_arr[],
    uniform float final_main[],
    uniform unsigned int32 count,
    uniform float delta,
    uniform float sum_shrink_weight)
{
    uniform float inv = delta / sum_shrink_weight;
    foreach (i = 0 ... count) {
        float bm = base_main[i];
        float w  = shrink_arr[i] * select(bm > 0.0f, bm, 1.0f);
        float s  = bm + w * inv;
        final_main[i] = max(s, 0.0f);
    }
}

// ─── F: flex occupied_main sum (sum of 3 arrays) ───
export uniform float xent_ispc_sum_outer_main(
    const uniform float final_main[],
    const uniform float margin_lead[],
    const uniform float margin_trail[],
    uniform unsigned int32 count)
{
    float p = 0.0f;
    foreach (i = 0 ... count) {
        p += final_main[i] + margin_lead[i] + margin_trail[i];
    }
    return reduce_add(p);
}

// ─── G: swiftstack masked add scalar (spacer distribution) ───
// For each i where mask[i] != 0:  values[i] += addend
export void xent_ispc_masked_add_f32(uniform float values[],
                                      const uniform unsigned int8 mask[],
                                      uniform unsigned int32 count,
                                      uniform float addend) {
    foreach (i = 0 ... count) {
        if (mask[i] != 0) {
            values[i] = values[i] + addend;
        }
    }
}

// ─── G: swiftstack priority-weighted distribution ───
// For each i where mask[i] != 0:
//   values[i] += remainder * (weights[i] / weight_sum)
export void xent_ispc_masked_fma_f32(uniform float values[],
                                      const uniform unsigned int8 mask[],
                                      const uniform float weights[],
                                      uniform unsigned int32 count,
                                      uniform float remainder,
                                      uniform float weight_sum) {
    uniform float factor = remainder / weight_sum;
    foreach (i = 0 ... count) {
        if (mask[i] != 0) {
            values[i] = values[i] + weights[i] * factor;
        }
    }
}

// ─── H: swiftstack proportional reduce (indirect via indices) ───
// For i in [0, count):
//   idx = indices[i]; current = values[idx]
//   share = min(reduce_amount * current / total, current)
//   values[idx] = current - share
export void xent_ispc_proportional_reduce_gather(
    uniform float values[],
    const uniform unsigned int32 indices[],
    uniform unsigned int32 count,
    uniform float reduce_amount,
    uniform float total)
{
    uniform float factor = reduce_amount / total;
    foreach (i = 0 ... count) {
        unsigned int32 idx = indices[i];
        float current = values[idx];
        float share = current * factor;
        if (share > current) share = current;
        values[idx] = current - share;
    }
}
