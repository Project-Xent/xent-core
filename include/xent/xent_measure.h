#ifndef XENT_MEASURE_H
#define XENT_MEASURE_H

#include "xent_types.h"

/* External-content measurement: HtmlView, images, and Components report
 * preferred sizes into layout without handing platform objects to Core.
 * The callback userdata is owned by the registrant; Core only stores the
 * pointer while the live node owns the registration. */

typedef struct XentExtMeasureReq {
	float           available_w;
	float           available_h;
	XentMeasureMode width_mode;
	XentMeasureMode height_mode;
} XentExtMeasureReq;

typedef bool (*XentExternalMeasureFn)(void *userdata, XentExtMeasureReq const *request, XentSize *out_size);

bool         xent_node_setmeasure(XentCtx *ctx, XentNodeId node, XentExternalMeasureFn fn, void *userdata);
bool         xent_node_clearmeasure(XentCtx *ctx, XentNodeId node);
bool         xent_node_hasmeasure(XentCtx const *ctx, XentNodeId node);


#endif
