#ifndef XENT_FOCUS_H
#define XENT_FOCUS_H

#include <stdint.h>

typedef struct XentNodeFocusStore {
	uint8_t *focusable;
	int32_t *tab_index;
} XentNodeFocusStore;

#endif
