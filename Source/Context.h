#ifndef _GLASS_CONTEXT_H
#define _GLASS_CONTEXT_H

#include "Utility.h"

#if defined(GLASS_NO_MERCY)
#define GLASS_context_setError(err) UNREACHABLE(#err)
#else
void GLASS_context_setError(GLenum error);
#endif // GLASS_NO_MERCY

/* Common HAS to be the first member of subcontexts */

typedef struct {
    CtxCommon common;
} CtxV2;

void GLASS_context_initV2(CtxV2* ctx, const glassInitParams* initParams, const glassSettings* settings);
void GLASS_context_cleanupV2(CtxV2* ctx);

CtxCommon* GLASS_context_getCommon(void);

INLINE CtxV2* GLASS_context_getV2(void) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ASSERT(ctx->initParams.version == GLASS_VERSION_2_0);
    return (CtxV2*)ctx;
}

void GLASS_context_bind(CtxCommon* ctx);
void GLASS_context_update(void);
void GLASS_context_setSwap(CtxCommon* ctx, bool swap);
void GLASS_context_waitSwap(CtxCommon* ctx);

#endif /* _GLASS_CONTEXT_H */