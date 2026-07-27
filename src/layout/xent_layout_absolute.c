#include "../xent_internal.h"

void xent_layout_node_absolute(XentLayoutRequest const *request) {
	XentCtx   *ctx         = request->ctx;
	XentNodeId node        = request->node;
	float      available_w = request->available_w;
	float      available_h = request->available_h;
	float      origin_x    = request->origin_x;
	float      origin_y    = request->origin_y;
	float      width       = 0.0f;
	float      height      = 0.0f;
	xent_decide_node_box(
	  ctx, node, available_w, available_h, request->definite_w, request->definite_h, &width, &height
	);

	float const x = origin_x + ctx->nodes.layout.abs_pos_x [xent_node_index(node)];
	float const y = origin_y + ctx->nodes.layout.abs_pos_y [xent_node_index(node)];

	ctx->nodes.layout.proposed_w [xent_node_index(node)] = available_w;
	ctx->nodes.layout.proposed_h [xent_node_index(node)] = available_h;
	ctx->nodes.layout.decided_w [xent_node_index(node)]  = width;
	ctx->nodes.layout.decided_h [xent_node_index(node)]  = height;
	ctx->nodes.layout.abs_x [xent_node_index(node)]      = x;
	ctx->nodes.layout.abs_y [xent_node_index(node)]      = y;
	xent_quantize_node_layout(ctx, node);

	width          = ctx->nodes.layout.decided_w [xent_node_index(node)];
	height         = ctx->nodes.layout.decided_h [xent_node_index(node)];
	float const qx = ctx->nodes.layout.abs_x [xent_node_index(node)];
	float const qy = ctx->nodes.layout.abs_y [xent_node_index(node)];

	float       content_w
	  = width
	  - (ctx->nodes.layout.padding_l [xent_node_index(node)] + ctx->nodes.layout.padding_r [xent_node_index(node)]);
	float content_h
	  = height
	  - (ctx->nodes.layout.padding_t [xent_node_index(node)] + ctx->nodes.layout.padding_b [xent_node_index(node)]);
	if (content_w < 0.0f) content_w = 0.0f;
	if (content_h < 0.0f) content_h = 0.0f;

	float      content_x = qx + ctx->nodes.layout.padding_l [xent_node_index(node)];
	float      content_y = qy + ctx->nodes.layout.padding_t [xent_node_index(node)];

	XentNodeId child     = ctx->nodes.topology.first_child [xent_node_index(node)];
	while (child != XENT_NODE_INVALID) {
		xent_layout_dispatch_node(&(XentLayoutRequest) {ctx, child, content_w, content_h, content_x, content_y});
		child = ctx->nodes.topology.next_sibling [xent_node_index(child)];
	}
}
