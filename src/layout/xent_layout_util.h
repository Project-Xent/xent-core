#ifndef XENT_LAYOUT_UTIL_H
#define XENT_LAYOUT_UTIL_H

/* When min_v > max_v (contradictory constraints) min wins, matching the CSS
 * used-value rule. */
static float inline xent_clampf(float value, float min_v, float max_v) {
	if (value < min_v) return min_v;
	if (value > max_v) return max_v;
	return value;
}

#endif
