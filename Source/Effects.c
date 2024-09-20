#include "Context.h"
#include "Utility.h"

#define IS_BLEND_FUNC(func) \
  (((func) == GL_ZERO) || ((func) == GL_ONE) || ((func) == GL_SRC_COLOR) || ((func) == GL_ONE_MINUS_SRC_COLOR) || ((func) == GL_SRC_ALPHA) || \
   ((func) == GL_ONE_MINUS_SRC_ALPHA) || ((func) == GL_DST_ALPHA) || ((func) == GL_ONE_MINUS_DST_ALPHA) || ((func) == GL_DST_COLOR) || \
   ((func) == GL_ONE_MINUS_DST_COLOR) || ((func) == GL_CONSTANT_COLOR) || ((func) == GL_ONE_MINUS_CONSTANT_COLOR) || ((func) == GL_CONSTANT_ALPHA) || \
   ((func) == GL_ONE_MINUS_CONSTANT_ALPHA) || ((func) == GL_SRC_ALPHA_SATURATE))

#define IS_CULL_FACE(mode) \
  (((mode) == GL_FRONT) || ((mode) == GL_BACK) || ((mode) == GL_FRONT_AND_BACK))

#define IS_EQUATION(mode) \
  (((mode) == GL_FUNC_ADD) || ((mode) == GL_MIN) || ((mode) == GL_MAX) || ((mode) == GL_FUNC_SUBTRACT) || ((mode) == GL_FUNC_REVERSE_SUBTRACT))

#define IS_FRONT_FACE(mode) (((mode) == GL_CW) || ((mode) == GL_CCW))

#define IS_LOGIC_OP(opcode) \
  (((opcode) == GL_CLEAR) || ((opcode) == GL_AND) || ((opcode) == GL_AND_REVERSE) || ((opcode) == GL_COPY) || ((opcode) == GL_AND_INVERTED) || ((opcode) == GL_NOOP) || \
   ((opcode) == GL_XOR) || ((opcode) == GL_OR) || ((opcode) == GL_NOR) || ((opcode) == GL_EQUIV) || ((opcode) == GL_INVERT) || ((opcode) == GL_OR_REVERSE) || ((opcode) == GL_COPY_INVERTED) || \
   ((opcode) == GL_OR_INVERTED) || ((opcode) == GL_NAND) || ((opcode) == GL_SET))

#define IS_STENCIL_OP(op) \
  (((op) == GL_KEEP) || ((op) == GL_ZERO) || ((op) == GL_REPLACE) || ((op) == GL_INCR) || ((op) == GL_INCR_WRAP) || \
  ((op) == GL_DECR) || ((op) == GL_DECR_WRAP) || ((op) == GL_INVERT))

#define IS_TEST_FUNC(func) \
  (((func) == GL_NEVER) || ((func) == GL_LESS) || ((func) == GL_EQUAL) || ((func) == GL_LEQUAL) || ((func) == GL_GREATER) || \
   ((func) == GL_NOTEQUAL) || ((func) == GL_GEQUAL) || ((func) == GL_ALWAYS))

void glAlphaFunc(GLenum func, GLclampf ref) {
    if (!IS_TEST_FUNC(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->alphaFunc != func) {
        ctx->alphaFunc = func;
        if (ctx->alphaTest)
            ctx->flags |= CONTEXT_FLAG_ALPHA;
    }
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();

    u32 blendColor = (u32)(0xFF * CLAMP_FLOAT(red)) << 24;
    blendColor |= (u32)(0xFF * CLAMP_FLOAT(green)) << 16;
    blendColor |= (u32)(0xFF * CLAMP_FLOAT(blue)) << 8;
    blendColor |= (u32)(0xFF * CLAMP_FLOAT(alpha));

    if (ctx->blendColor != blendColor) {
        ctx->blendColor = blendColor;
        if (ctx->blendMode)
            ctx->flags |= CONTEXT_FLAG_BLEND;
    }
}

void glBlendEquation(GLenum mode) { glBlendEquationSeparate(mode, mode); }

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
  if (!IS_EQUATION(modeRGB) || !IS_EQUATION(modeAlpha)) {
    GLASS_context_setError(GL_INVALID_ENUM);
    return;
  }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->blendEqRGB != modeRGB || ctx->blendEqAlpha != modeAlpha) {
        ctx->blendEqRGB = modeRGB;
        ctx->blendEqAlpha = modeAlpha;
        if (ctx->blendMode)
            ctx->flags |= CONTEXT_FLAG_BLEND;
    }
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) { glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor); }

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    if (!IS_BLEND_FUNC(srcRGB) || !IS_BLEND_FUNC(dstRGB) || !IS_BLEND_FUNC(srcAlpha) || !IS_BLEND_FUNC(dstAlpha)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->blendSrcRGB != srcRGB || ctx->blendDstRGB != dstRGB || ctx->blendSrcAlpha != srcAlpha || ctx->blendDstAlpha != dstAlpha) {
        ctx->blendSrcRGB = srcRGB;
        ctx->blendDstRGB = dstRGB;
        ctx->blendSrcAlpha = srcAlpha;
        ctx->blendDstAlpha = dstAlpha;
        if (ctx->blendMode)
            ctx->flags |= CONTEXT_FLAG_BLEND;
    }
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if ((ctx->writeRed != (red == GL_TRUE)) || (ctx->writeGreen != (green == GL_TRUE)) || (ctx->writeBlue != (blue == GL_TRUE)) || (ctx->writeAlpha != (alpha == GL_TRUE))) {
        ctx->writeRed = red == GL_TRUE;
        ctx->writeGreen = green == GL_TRUE;
        ctx->writeBlue = blue == GL_TRUE;
        ctx->writeAlpha = alpha == GL_TRUE;
        ctx->flags |= CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glCullFace(GLenum mode) {
    if (!IS_CULL_FACE(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->cullFaceMode != mode) {
        ctx->cullFaceMode = mode;
        if (ctx->cullFace)
            ctx->flags |= CONTEXT_FLAG_CULL_FACE;
    }
}

void glDepthFunc(GLenum func) {
    if (!IS_TEST_FUNC(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->depthFunc != func) {
        ctx->depthFunc = func;
        if (ctx->depthTest)
            ctx->flags |= CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glDepthMask(GLboolean flag) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->writeDepth == (flag == GL_TRUE)) {
        ctx->writeDepth = flag == GL_TRUE;
        ctx->flags |= CONTEXT_FLAG_COLOR_DEPTH;
    }
}

void glDepthRangef(GLclampf nearVal, GLclampf farVal) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->depthNear = CLAMP_FLOAT(nearVal);
    ctx->depthFar = CLAMP_FLOAT(farVal);
    if (ctx->depthTest)
        ctx->flags |= CONTEXT_FLAG_DEPTHMAP;
}

void glFrontFace(GLenum mode) {
    if (!IS_FRONT_FACE(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->frontFaceMode != mode) {
        ctx->frontFaceMode = mode;
        if (ctx->cullFace)
            ctx->flags |= CONTEXT_FLAG_CULL_FACE;
    }
}

void glLogicOp(GLenum opcode) {
    if (!IS_LOGIC_OP(opcode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->logicOp != opcode) {
        ctx->logicOp = opcode;
        if (!ctx->blendMode)
            ctx->flags |= CONTEXT_FLAG_BLEND;
    }
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->polygonFactor = factor;
    ctx->polygonUnits = units;
    if (ctx->depthTest && ctx->polygonOffset)
        ctx->flags |= CONTEXT_FLAG_DEPTHMAP;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (width < 0 || height < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->scissorX != x || ctx->scissorY != y || ctx->scissorW != width || ctx->scissorH != height) {
        ctx->scissorX = x;
        ctx->scissorY = y;
        ctx->scissorW = width;
        ctx->scissorH = height;

        if (ctx->scissorMode != GPU_SCISSOR_DISABLE)
            ctx->flags |= CONTEXT_FLAG_SCISSOR;
    }
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (!IS_TEST_FUNC(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->stencilFunc != func || ctx->stencilRef != ref || ctx->stencilMask != mask) {
        ctx->stencilFunc = func;
        ctx->stencilRef = ref;
        ctx->stencilMask = mask;
        if (ctx->stencilTest)
            ctx->flags |= CONTEXT_FLAG_STENCIL;
    }
}

void glStencilMask(GLuint mask) {
    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->stencilWriteMask != mask) {
        ctx->stencilWriteMask = mask;
        if (ctx->stencilTest)
            ctx->flags |= CONTEXT_FLAG_STENCIL;
    }
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (!IS_STENCIL_OP(sfail) || !IS_STENCIL_OP(dpfail) || !IS_STENCIL_OP(dppass)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->stencilFail != sfail || ctx->stencilDepthFail != dpfail || ctx->stencilPass != dppass) {
        ctx->stencilFail = sfail;
        ctx->stencilDepthFail = dpfail;
        ctx->stencilPass = dppass;
        if (ctx->stencilTest)
            ctx->flags |= CONTEXT_FLAG_STENCIL;
    }
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (width < 0 || height < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    if (ctx->viewportX != x || ctx->viewportY != y || ctx->viewportW != width || ctx->viewportH != height) {
        ctx->viewportX = x;
        ctx->viewportY = y;
        ctx->viewportW = width;
        ctx->viewportH = height;
        ctx->scissorMode = GPU_SCISSOR_DISABLE;
        ctx->flags |= (CONTEXT_FLAG_VIEWPORT | CONTEXT_FLAG_SCISSOR);
    }
}