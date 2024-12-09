#ifndef _GLASS_CONTEXT_H
#define _GLASS_CONTEXT_H

#include "Utility.h"

void GLASS_context_initCommon(CtxCommon* ctx, const glassInitParams* initParams, const glassSettings* settings);
void GLASS_context_cleanupCommon(CtxCommon* ctx);
CtxCommon* GLASS_context_getCommon(void);

void GLASS_context_bind(CtxCommon* ctx);
void GLASS_context_flush(void);

#if defined(GLASS_NO_MERCY)
#define GLASS_context_setError(err) UNREACHABLE(#err)
#else
void GLASS_context_setError(GLenum error);
#endif // GLASS_NO_MERCY

#endif /* _GLASS_CONTEXT_H */