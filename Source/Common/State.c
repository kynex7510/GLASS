#include "Base/Context.h"

#define VENDOR "Kynex7510"
#define RENDERER "GLASS"
#define VERSION_2_0 "OpenGL ES 2.0"
#define SHADING_LANGUAGE_VERSION "SHBIN 1.0"
#define EXTENSIONS_2_0 ""

static void GLASS_setCapability(GLenum cap, bool enabled) {
    CtxCommon* ctx = GLASS_context_getBound();

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
            ctx->scissorMode = (enabled ? SCISSORMODE_NORMAL : SCISSORMODE_DISABLE);
            ctx->flags |= GLASS_CONTEXT_FLAG_SCISSOR;
            break;
        case GL_SCISSOR_TEST_INVERTED_PICA:
            ctx->scissorMode = (enabled ? SCISSORMODE_INVERTED : SCISSORMODE_DISABLE);
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
    CtxCommon* ctx = GLASS_context_getBound();

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
            return ctx->scissorMode == SCISSORMODE_NORMAL ? GL_TRUE : GL_FALSE;
        case GL_SCISSOR_TEST_INVERTED_PICA:
            return ctx->scissorMode == SCISSORMODE_INVERTED ? GL_TRUE : GL_FALSE;
        case GL_STENCIL_TEST:
            return ctx->stencilTest ? GL_TRUE : GL_FALSE;
    }

    GLASS_context_setError(GL_INVALID_ENUM);
    return GL_FALSE;
}

GLenum glGetError(void) {
    CtxCommon* ctx = GLASS_context_getBound();
    GLenum error = ctx->lastError;
    ctx->lastError = GL_NO_ERROR;
    return error;
}

static inline const GLubyte* getVersionString(u8 version) {
    switch (version) {
        case GLASS_VERSION_2_0:
            return (const GLubyte*)VERSION_2_0;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return NULL;
}

static inline const GLubyte* getExtensionsString(u8 version) {
    switch (version) {
        case GLASS_VERSION_2_0:
            return (const GLubyte*)EXTENSIONS_2_0;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return NULL;
}

const GLubyte* glGetString(GLenum name) {
    CtxCommon* ctx = GLASS_context_getBound();

    switch (name) {
        case GL_VENDOR:
            return (const GLubyte*)VENDOR;
        case GL_RENDERER:
#ifndef NDEBUG
            return(const GLubyte*)RENDERER;
#else
            return (const GLubyte*)RENDERER " (DEBUG)";
#endif
        case GL_VERSION:
            return getVersionString(ctx->initParams.version);
        case GL_SHADING_LANGUAGE_VERSION:
            return (const GLubyte*)SHADING_LANGUAGE_VERSION;
        case GL_EXTENSIONS:
            return getExtensionsString(ctx->initParams.version);
    }

    GLASS_context_setError(GL_INVALID_ENUM);
    return NULL;
}