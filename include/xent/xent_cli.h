#ifndef XENT_CLI_H
#define XENT_CLI_H

#include "xent_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void xent_dump_tree(XentContext const *ctx, XentNodeId root, FILE *out);
void xent_dump_layout_text(XentContext const *ctx, XentNodeId root, FILE *out);
void xent_dump_semantics_text(XentContext const *ctx, XentNodeId root, FILE *out);
bool xent_dump_layout_json(XentContext const *ctx, XentNodeId root, FILE *out);

#ifdef __cplusplus
}
#endif

#endif
