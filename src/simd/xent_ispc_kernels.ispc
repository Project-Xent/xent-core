export uniform float xent_ispc_sum_f32(uniform const float values [], uniform unsigned int32 count) {
	float partial = 0.0f;
	foreach (i = 0 ... count) { partial += values [i]; }
	return reduce_add(partial);
}

export uniform bool xent_ispc_probe() { return true; }

export void         xent_ispc_quantize_f32(uniform float values [], uniform unsigned int32 count, uniform float scale) {
	uniform float inv_scale = 1.0f / scale;
	foreach (i = 0 ... count) { values [i] = round(values [i] * scale) * inv_scale; }
}

export void xent_ispc_quantize_rects(
  uniform float x [], uniform float y [], uniform float w [], uniform float h [], uniform unsigned int32 count,
  uniform float scale
) {
	uniform float inv_scale = 1.0f / scale;
	foreach (i = 0 ... count) {
		float rx = round(x [i] * scale) * inv_scale;
		float ry = round(y [i] * scale) * inv_scale;
		if (isfinite(w [i])) w [i] = round((x [i] + w [i]) * scale) * inv_scale - rx;
		if (isfinite(h [i])) h [i] = round((y [i] + h [i]) * scale) * inv_scale - ry;
		x [i] = rx;
		y [i] = ry;
	}
}

export void xent_ispc_or_u32_masked(
  uniform unsigned int32 flags [], uniform const unsigned int8 mask [], uniform unsigned int32 count,
  uniform unsigned int32 or_value
) {
	foreach (i = 0 ... count) {
		if (mask [i] != 0) flags [i] = flags [i] | or_value;
	}
}

export void xent_ispc_zero_u8_masked(
  uniform unsigned int8 out [], uniform const unsigned int8 mask [], uniform unsigned int32 count
) {
	foreach (i = 0 ... count) {
		if (mask [i] != 0) out [i] = 0;
	}
}

export void xent_ispc_scatter_zero_u32(
  uniform unsigned int32 flags [], uniform const unsigned int32 indices [], uniform unsigned int32 count
) {
	foreach (i = 0 ... count) {
		unsigned int32 idx = indices [i];
		flags [idx]        = 0;
	}
}

export void xent_ispc_flex_line_stats(
  uniform const float base_main [], uniform const float margin_lead [], uniform const float margin_trail [],
  uniform const float base_cross [], uniform const float margin_cross_lead [],
  uniform const float margin_cross_trail [], uniform const float grow [], uniform const float shrink [],
  uniform unsigned int32 count, uniform float *uniform out_sum_outer_main, uniform float *uniform out_sum_grow,
  uniform float *uniform out_sum_shrink_weight, uniform float *uniform out_max_cross_outer
) {
	float p_outer = 0.0f;
	float p_grow  = 0.0f;
	float p_sw    = 0.0f;
	float p_cross = 0.0f;
	foreach (i = 0 ... count) {
		p_outer  += base_main [i] + margin_lead [i] + margin_trail [i];
		p_grow   += grow [i];
		float bm  = base_main [i];
		p_sw     += shrink [i] * select(bm > 0.0f, bm, 1.0f);
		p_cross   = max(p_cross, base_cross [i] + margin_cross_lead [i] + margin_cross_trail [i]);
	}
	*out_sum_outer_main    = reduce_add(p_outer);
	*out_sum_grow          = reduce_add(p_grow);
	*out_sum_shrink_weight = reduce_add(p_sw);
	*out_max_cross_outer   = reduce_max(p_cross);
}

export void xent_ispc_flex_distribute_grow(
  uniform const float base_main [], uniform const float grow_arr [], uniform float final_main [],
  uniform unsigned int32 count, uniform float delta, uniform float sum_grow
) {
	uniform float inv = delta / sum_grow;
	foreach (i = 0 ... count) { final_main [i] = base_main [i] + grow_arr [i] * inv; }
}

export void xent_ispc_flex_distribute_shrink(
  uniform const float base_main [], uniform const float shrink_arr [], uniform float final_main [],
  uniform unsigned int32 count, uniform float delta, uniform float sum_shrink_weight
) {
	uniform float inv = delta / sum_shrink_weight;
	foreach (i = 0 ... count) {
		float bm       = base_main [i];
		float w        = shrink_arr [i] * select(bm > 0.0f, bm, 1.0f);
		float s        = bm + w * inv;
		final_main [i] = max(s, 0.0f);
	}
}

export uniform float xent_ispc_sum_outer_main(
  uniform const float final_main [], uniform const float margin_lead [], uniform const float margin_trail [],
  uniform unsigned int32 count
) {
	float p = 0.0f;
	foreach (i = 0 ... count) { p += final_main [i] + margin_lead [i] + margin_trail [i]; }
	return reduce_add(p);
}

export void xent_ispc_masked_add_f32(
  uniform float values [], uniform const unsigned int8 mask [], uniform unsigned int32 count, uniform float addend
) {
	foreach (i = 0 ... count) {
		if (mask [i] != 0) values [i] = values [i] + addend;
	}
}

export void xent_ispc_masked_fma_f32(
  uniform float values [], uniform const unsigned int8 mask [], uniform const float weights [],
  uniform unsigned int32 count, uniform float remainder, uniform float weight_sum
) {
	uniform float factor = remainder / weight_sum;
	foreach (i = 0 ... count) {
		if (mask [i] != 0) values [i] = values [i] + weights [i] * factor;
	}
}

export void xent_ispc_proportional_reduce_gather(
  uniform float values [], uniform const unsigned int32 indices [], uniform unsigned int32 count,
  uniform float reduce_amount, uniform float total
) {
	uniform float factor = reduce_amount / total;
	foreach (i = 0 ... count) {
		unsigned int32 idx     = indices [i];
		float          current = values [idx];
		float          share   = current * factor;
		if (share > current) share = current;
		values [idx] = current - share;
	}
}
