#ifndef _GLASS_CONTEXT_H
#define _GLASS_CONTEXT_H

#include "Utility.h"

/* Common HAS to be the first member of subcontexts */

typedef struct {
    CtxCommon common;
} CtxV1;

void GLASS_context_initV1(CtxV1* ctx, const glassSettings* settings);
void GLASS_context_cleanupV1(CtxV1* ctx);

CtxCommon* GLASS_context_getCommon(void);

INLINE CtxV1* GLASS_context_getV1(void) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ASSERT(ctx->version == GLASS_VERSION_1_1);
    return (CtxV1*)ctx;
}

void GLASS_context_bind(CtxCommon* ctx);
void GLASS_context_update(void);
void GLASS_context_setError(GLenum error);
void GLASS_context_setSwap(CtxCommon* ctx, bool swap);
void GLASS_context_waitSwap(CtxCommon* ctx);

#endif /* _GLASS_CONTEXT_H */