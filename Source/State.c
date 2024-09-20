#include "Context.h"

GLenum glGetError(void) {
    CtxCommon* ctx = GLASS_context_getCommon();
    GLenum error = ctx->lastError;
    ctx->lastError = GL_NO_ERROR;
    return error;
}