#include <stdlib.h>
#include <memory.h>

#include "util.h"

void *mallocz(cbz_ctx_t *ctx, size_t size)
{
    void *p = ctx->malloc(ctx->handle, size);
    if (p != NULL)
        memset(p, 0, size);
    return p;
}
