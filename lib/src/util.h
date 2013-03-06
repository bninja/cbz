#ifndef _CBZ_UTIL_H_INCLUDED_
#define _CBZ_UTIL_H_INCLUDED_

#include "cbz.h"

#define MALLOC(size)        mallocz(ctx, size)
#define NEW(type)            MALLOC(sizeof(type))
#define CALLOC(num, size)    ctx->calloc(ctx->handle, num, size)
#define NEWA(num, type)        CALLOC(num, sizeof(type))
#define REALLOC(p, size)    ctx->realloc(ctx->handle, p, size)
#define FREE(p)                ctx->free(ctx->handle, p); p = NULL

void *mallocz(cbz_ctx_t *ctx, size_t size);

#define LOG_DBG(fmt, ...)    if (ctx->log != NULL) ctx->log(ctx->handle, CBZ_LOG_DBG, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...)    if (ctx->log != NULL) ctx->log(ctx->handle, CBZ_LOG_INFO, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...)    if (ctx->log != NULL) ctx->log(ctx->handle, CBZ_LOG_WARN, fmt, __VA_ARGS__)
#define LOG_ERR(fmt, ...)    if (ctx->log != NULL) ctx->log(ctx->handle, CBZ_LOG_ERR, fmt, __VA_ARGS__)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
