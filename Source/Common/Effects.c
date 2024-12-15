#include "Base/Context.h"
#include "Base/Utility.h"

static bool GLASS_isTestFunc(GLenum func) {
    switch (func) {
        case GL_NEVER:
        case GL_LESS:
        case GL_EQUAL:
        case GL_LEQUAL:
        case GL_GREATER:
        case GL_NOTEQUAL:
        case GL_GEQUAL:
        case GL_ALWAYS:
            return true;
    }

    return false;
}

static bool GLASS_isEquation(GLenum mode) {
    switch (mode) {
        case GL_FUNC_ADD:
        case GL_MIN:
        case GL_MAX:
        case GL_FUNC_SUBTRACT:
        case GL_FUNC_REVERSE_SUBTRACT:
            return true;
    }

    return false;
}

static bool GLASS_isBlendFunc(GLenum func) {
    switch (func) {
        case GL_ZERO:
        case GL_ONE:
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_CONSTANT_COLOR:
        case GL_ONE_MINUS_CONSTANT_COLOR:
        case GL_CONSTANT_ALPHA:
        case GL_ONE_MINUS_CONSTANT_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            return true;
    }

    return false;
}

static bool GLASS_isCullFace(GLenum mode) {
    switch (mode) {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            return true;
    }

    return false;
}

static bool GLASS_isFrontFace(GLenum mode) {
    switch (mode) {
        case GL_CW:
        case GL_CCW:
            return true;
    }

    return false;
}

static bool GLASS_isLogicOp(GLenum opcode) {
    switch (opcode) {
        case GL_CLEAR:
        case GL_AND:
        case GL_AND_REVERSE:
        case GL_COPY:
        case GL_AND_INVERTED:
        case GL_NOOP:
        case GL_XOR:
        case GL_OR:
        case GL_NOR:
        case GL_EQUIV:
        case GL_INVERT:
        case GL_OR_REVERSE:
        case GL_COPY_INVERTED:
        case GL_OR_INVERTED:
        case GL_NAND:
        case GL_SET:
            return true;
    }

    return false;
}

static bool GLASS_isStencilOp(GLenum op) {
    switch (op) {
        case GL_KEEP:
        case GL_ZERO:
        case GL_REPLACE:
        case GL_INCR:
        case GL_INCR_WRAP:
        case GL_DECR:
        case GL_DECR_WRAP:
        case GL_INVERT:
            return true;
    }

    return false;
}

void glAlphaFunc(GLenum func, GLclampf ref) {
    if (!GLASS_isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->alphaFunc != func) {
        ctx->alphaFunc = func;
        if (ctx->alphaTest)
            ctx->flags |= GLASS_CONTEXT_FLAG_ALPHA;
    }
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();

    u32 blendColor = ((u32)(0xFF * CLAMP(0.0f, 1.0f, red)) << 24);
    blendColor |= ((u32)(0xFF * CLAMP(0.0f, 1.0f, green)) << 16);
    blendColor |= ((u32)(0xFF * CLAMP(0.0f, 1.0f, blue)) << 8);
    blendColor |= ((u32)(0xFF * CLAMP(0.0f, 1.0f, alpha)));

    if (ctx->blendColor != blendColor) {
        ctx->blendColor = blendColor;
        if (ctx->blendMode)
            ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
    }
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
  if (!GLASS_isEquation(modeRGB) || !GLASS_isEquation(modeAlpha)) {
    GLASS_context_setError(GL_INVALID_ENUM);
    return;
  }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->blendEqRGB != modeRGB) || (ctx->blendEqAlpha != modeAlpha)) {
        ctx->blendEqRGB = modeRGB;
        ctx->blendEqAlpha = modeAlpha;
        if (ctx->blendMode)
            ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
    }
}

void glBlendEquation(GLenum mode) { glBlendEquationSeparate(mode, mode); }

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    if (!GLASS_isBlendFunc(srcRGB) || !GLASS_isBlendFunc(dstRGB) || !GLASS_isBlendFunc(srcAlpha) || !GLASS_isBlendFunc(dstAlpha)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->blendSrcRGB != srcRGB) || (ctx->blendDstRGB != dstRGB) || (ctx->blendSrcAlpha != srcAlpha) || (ctx->blendDstAlpha != dstAlpha)) {
        ctx->blendSrcRGB = srcRGB;
        ctx->blendDstRGB = dstRGB;
        ctx->blendSrcAlpha = srcAlpha;
        ctx->blendDstAlpha = dstAlpha;
        if (ctx->blendMode)
            ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
    }
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) { glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor); }

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->writeRed != (red == GL_TRUE)) || (ctx->writeGreen != (green == GL_TRUE)) || (ctx->writeBlue != (blue == GL_TRUE)) || (ctx->writeAlpha != (alpha == GL_TRUE))) {
        ctx->writeRed = red == GL_TRUE;
        ctx->writeGreen = green == GL_TRUE;
        ctx->writeBlue = blue == GL_TRUE;
        ctx->writeAlpha = alpha == GL_TRUE;
        ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glCullFace(GLenum mode) {
    if (!GLASS_isCullFace(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->cullFaceMode != mode) {
        ctx->cullFaceMode = mode;
        if (ctx->cullFace)
            ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;
    }
}

void glDepthFunc(GLenum func) {
    if (!GLASS_isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->depthFunc != func) {
        ctx->depthFunc = func;
        if (ctx->depthTest)
            ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glDepthMask(GLboolean flag) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->writeDepth == (flag == GL_TRUE)) {
        ctx->writeDepth = flag == GL_TRUE;
        ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glDepthRangef(GLclampf nearVal, GLclampf farVal) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->depthNear = CLAMP(0.0f, 1.0f, nearVal);
    ctx->depthFar = CLAMP(0.0f, 1.0f, farVal);
    if (ctx->depthTest)
        ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;
}

void glFrontFace(GLenum mode) {
    if (!GLASS_isFrontFace(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->frontFaceMode != mode) {
        ctx->frontFaceMode = mode;
        if (ctx->cullFace)
            ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;
    }
}

void glLogicOp(GLenum opcode) {
    if (!GLASS_isLogicOp(opcode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->logicOp != opcode) {
        ctx->logicOp = opcode;
        if (!ctx->blendMode)
            ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
    }
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->polygonFactor = factor;
    ctx->polygonUnits = units;
    if (ctx->depthTest && ctx->polygonOffset)
        ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;
}

static GLsizei GLASS_screenWidth(CtxCommon* ctx) {
    if (ctx->settings.targetScreen == GFX_TOP)
        return gfxIsWide() ? 800 : 400;

    return 320;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((width < 0) || (height < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->scissorX != x) || (ctx->scissorY != y) || (ctx->scissorW != width) || (ctx->scissorH != height)) {
        // Account for rotated screens.
        ctx->scissorX = (GLASS_screenWidth(ctx) - (x + width));
        ctx->scissorY = y;
        ctx->scissorW = width;
        ctx->scissorH = height;

        if (ctx->scissorMode != GPU_SCISSOR_DISABLE)
            ctx->flags |= GLASS_CONTEXT_FLAG_SCISSOR;
    }
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (!GLASS_isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->stencilFunc != func) || (ctx->stencilRef != ref) || (ctx->stencilMask != mask)) {
        ctx->stencilFunc = func;
        ctx->stencilRef = ref;
        ctx->stencilMask = mask;
        if (ctx->stencilTest)
            ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
    }
}

void glStencilMask(GLuint mask) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->stencilWriteMask != mask) {
        ctx->stencilWriteMask = mask;
        if (ctx->stencilTest)
            ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
    }
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (!GLASS_isStencilOp(sfail) || !GLASS_isStencilOp(dpfail) || !GLASS_isStencilOp(dppass)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->stencilFail != sfail) || (ctx->stencilDepthFail != dpfail) || (ctx->stencilPass != dppass)) {
        ctx->stencilFail = sfail;
        ctx->stencilDepthFail = dpfail;
        ctx->stencilPass = dppass;
        if (ctx->stencilTest)
            ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
    }
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((width < 0) || (height < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->viewportX != x) || (ctx->viewportY != y) || (ctx->viewportW != width) || (ctx->viewportH != height)) {
        // Account for rotated screens.
        ctx->viewportX = (GLASS_screenWidth(ctx) - (x + width));
        ctx->viewportY = y;
        ctx->viewportW = width;
        ctx->viewportH = height;
        ctx->scissorMode = GPU_SCISSOR_DISABLE;
        ctx->flags |= (GLASS_CONTEXT_FLAG_VIEWPORT | GLASS_CONTEXT_FLAG_SCISSOR);
    }
}