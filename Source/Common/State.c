#include "Base/Context.h"

#define MAKE_STRING(s) (const GLubyte*)#s

static void GLASS_setCapability(GLenum cap, bool enabled) {
    CtxCommon* ctx = GLASS_context_getCommon();

    switch (cap) {
        case GL_ALPHA_TEST:
            ctx->alphaTest = enabled;
            ctx->flags |= GLASS_CONTEXT_FLAG_ALPHA;
            break;
        case GL_BLEND:
        case GL_COLOR_LOGIC_OP:
            ctx->blendMode = (enabled ? (cap == GL_BLEND) : (cap == GL_COLOR_LOGIC_OP));
            ctx->flags |= (GLASS_CONTEXT_FLAG_BLEND | GLASS_CONTEXT_FLAG_FRAGOP);
            break;
        case GL_CULL_FACE:
            ctx->cullFace = enabled;
            ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;
            break;
        case GL_DEPTH_TEST:
            ctx->depthTest = enabled;
            ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
            break;
        case GL_POLYGON_OFFSET_FILL:
            ctx->polygonOffset = enabled;
            ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;
            break;
        case GL_SCISSOR_TEST:
            ctx->scissorMode = (enabled ? GPU_SCISSOR_NORMAL : GPU_SCISSOR_DISABLE);
            ctx->flags |= GLASS_CONTEXT_FLAG_SCISSOR;
            break;
        case GL_SCISSOR_TEST_INVERTED_PICA:
            ctx->scissorMode = (enabled ? GPU_SCISSOR_INVERT : GPU_SCISSOR_DISABLE);
            ctx->flags |= GLASS_CONTEXT_FLAG_SCISSOR;
            break;
        case GL_STENCIL_TEST:
            ctx->stencilTest = enabled;
            ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }
}

void glDisable(GLenum cap) { GLASS_setCapability(cap, false); }
void glEnable(GLenum cap) { GLASS_setCapability(cap, true); }

GLboolean glIsEnabled(GLenum cap) {
    CtxCommon* ctx = GLASS_context_getCommon();

    switch (cap) {
        case GL_ALPHA_TEST:
            return ctx->alphaTest ? GL_TRUE : GL_FALSE;
        case GL_BLEND:
            return ctx->blendMode ? GL_TRUE : GL_FALSE;
        case GL_COLOR_LOGIC_OP:
            return !ctx->blendMode ? GL_TRUE : GL_FALSE;
        case GL_CULL_FACE:
            return ctx->cullFace ? GL_TRUE : GL_FALSE;
        case GL_DEPTH_TEST:
            return ctx->depthTest ? GL_TRUE : GL_FALSE;
        case GL_POLYGON_OFFSET_FILL:
            return ctx->polygonOffset ? GL_TRUE : GL_FALSE;
        case GL_SCISSOR_TEST:
            return ctx->scissorMode == GPU_SCISSOR_NORMAL ? GL_TRUE : GL_FALSE;
        case GL_SCISSOR_TEST_INVERTED_PICA:
            return ctx->scissorMode == GPU_SCISSOR_INVERT ? GL_TRUE : GL_FALSE;
        case GL_STENCIL_TEST:
            return ctx->stencilTest ? GL_TRUE : GL_FALSE;
    }

    GLASS_context_setError(GL_INVALID_ENUM);
    return GL_FALSE;
}

GLenum glGetError(void) {
    CtxCommon* ctx = GLASS_context_getCommon();
    GLenum error = ctx->lastError;
    ctx->lastError = GL_NO_ERROR;
    return error;
}

const GLubyte* glGetString(GLenum name) {
    switch (name) {
        case GL_VENDOR:
            return MAKE_STRING(Kynex7510);
        case GL_RENDERER:
#ifndef NDEBUG
            return MAKE_STRING(GLASS (Release));
#else
            return MAKE_STRING(GLASS (Debug));
#endif
        case GL_VERSION:
            return MAKE_STRING(OpenGL ES 2.0);
        case GL_SHADING_LANGUAGE_VERSION:
            return MAKE_STRING(SHBIN 1.0);
        case GL_EXTENSIONS:
            return MAKE_STRING();
    }

    GLASS_context_setError(GL_INVALID_ENUM);
    return NULL;
}