#ifndef XENT_LAYOUT_UTIL_H
#define XENT_LAYOUT_UTIL_H

static float inline xent_clampf(float value, float min_v, float max_v) {
	if (value < min_v) return min_v;
	if (value > max_v) return max_v;
	return value;
}

#endif
