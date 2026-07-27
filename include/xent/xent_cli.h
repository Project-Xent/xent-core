#ifndef XENT_CLI_H
#define XENT_CLI_H

#include <stdio.h>

#include "xent_types.h"


/* Direct diagnostics (not a plugin). Opt-in via this header; not part of xent.h. */

void xent_dump_tree(XentCtx const *ctx, XentNodeId root, FILE *out);
void xent_dump_layout_text(XentCtx const *ctx, XentNodeId root, FILE *out);
void xent_dump_semantics_text(XentCtx const *ctx, XentNodeId root, FILE *out);
bool xent_dump_layout_json(XentCtx const *ctx, XentNodeId root, FILE *out);


#endif
