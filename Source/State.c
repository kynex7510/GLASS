#include "Context.h"

static void GLASS_setCapability(GLenum cap, bool enabled) {
    CtxCommon* ctx = GLASS_context_getCommon();

    switch (cap) {
        case GL_CULL_FACE:
            ctx->cullFace = enabled;
            ctx->flags |= CONTEXT_FLAG_CULL_FACE;
            return;
        case GL_SCISSOR_TEST:
            ctx->scissorMode = (enabled ? GPU_SCISSOR_NORMAL : GPU_SCISSOR_DISABLE);
            ctx->flags |= CONTEXT_FLAG_SCISSOR;
            return;
        case GL_SCISSOR_TEST_INVERTED_PICA:
            ctx->scissorMode = (enabled ? GPU_SCISSOR_INVERT : GPU_SCISSOR_DISABLE);
            ctx->flags |= CONTEXT_FLAG_SCISSOR;
            return;
    }

    GLASS_context_setError(GL_INVALID_ENUM);
}

void glDisable(GLenum cap) { GLASS_setCapability(cap, false); }
void glEnable(GLenum cap) { GLASS_setCapability(cap, true); }

GLenum glGetError(void) {
    CtxCommon* ctx = GLASS_context_getCommon();
    GLenum error = ctx->lastError;
    ctx->lastError = GL_NO_ERROR;
    return error;
}