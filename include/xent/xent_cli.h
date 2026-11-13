#ifndef XENT_CLI_H
#define XENT_CLI_H

#include <stdio.h>

#include "xent_types.h"

void xent_dump_tree(XentCtx const *ctx, XentNodeId root, FILE *out);
void xent_dump_layouttext(XentCtx const *ctx, XentNodeId root, FILE *out);
void xent_dump_semanticstext(XentCtx const *ctx, XentNodeId root, FILE *out);
bool xent_dump_layoutjson(XentCtx const *ctx, XentNodeId root, FILE *out);

#endif
