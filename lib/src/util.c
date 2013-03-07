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

void *memmem(void *needle, size_t needle_len, void *haystack, size_t haystack_len)
{
    char *hc, *he, *hi;
    char *ne, *ni, *n;

    if (haystack_len < needle_len)
        return NULL;

    hc = haystack;
    he = hc + haystack_len - needle_len + 1;
    n = needle;
    ne = n + needle_len;

    while (hc != he) {
        if (*hc == *n) {
            hi = hc + 1;
            ni = n + 1;
            while (ni != ne) {
                if (*hi != *ni)
                    break;
                ni += 1;
            }
            if (ni == ne)
                return hc;
        }
        hc += 1;
    }

    return NULL;
}
