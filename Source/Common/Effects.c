/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Base/Context.h"
#include "Base/Math.h"

static inline bool isTestFunc(GLenum func) {
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

static inline bool isEquation(GLenum mode) {
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

static inline bool isBlendFunc(GLenum func) {
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

static inline bool isCullFace(GLenum mode) {
    switch (mode) {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            return true;
    }

    return false;
}

static inline bool isFrontFace(GLenum mode) {
    switch (mode) {
        case GL_CW:
        case GL_CCW:
            return true;
    }

    return false;
}

static inline bool isLogicOp(GLenum opcode) {
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

static inline bool isStencilOp(GLenum op) {
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
    if (!isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->alphaFunc = func;
    ctx->flags |= GLASS_CONTEXT_FLAG_ALPHA;
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getBound();

    u32 blendColor = ((u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, red)) << 24);
    blendColor |= ((u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, green)) << 16);
    blendColor |= ((u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, blue)) << 8);
    blendColor |= ((u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, alpha)));

    ctx->blendColor = blendColor;
    ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
  if (!isEquation(modeRGB) || !isEquation(modeAlpha)) {
    GLASS_context_setError(GL_INVALID_ENUM);
    return;
  }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->blendEqRGB = modeRGB;
    ctx->blendEqAlpha = modeAlpha;
    ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
}

void glBlendEquation(GLenum mode) { glBlendEquationSeparate(mode, mode); }

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    if (!isBlendFunc(srcRGB) || !isBlendFunc(dstRGB) || !isBlendFunc(srcAlpha) || !isBlendFunc(dstAlpha)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->blendSrcRGB = srcRGB;
    ctx->blendDstRGB = dstRGB;
    ctx->blendSrcAlpha = srcAlpha;
    ctx->blendDstAlpha = dstAlpha;
    ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) { glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor); }

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->writeRed = red == GL_TRUE;
    ctx->writeGreen = green == GL_TRUE;
    ctx->writeBlue = blue == GL_TRUE;
    ctx->writeAlpha = alpha == GL_TRUE;
    ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
}

void glCullFace(GLenum mode) {
    if (!isCullFace(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->cullFaceMode = mode;
    ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;
}

void glDepthFunc(GLenum func) {
    if (!isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->depthFunc = func;
    ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
}

void glDepthMask(GLboolean flag) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->writeDepth = flag == GL_TRUE;
    ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;
}

void glDepthRangef(GLclampf nearVal, GLclampf farVal) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->depthNear = GLASS_CLAMP(0.0f, 1.0f, nearVal);
    ctx->depthFar = GLASS_CLAMP(0.0f, 1.0f, farVal);
    ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;
}

void glFrontFace(GLenum mode) {
    if (!isFrontFace(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->frontFaceMode = mode;
    ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;
}

void glLogicOp(GLenum opcode) {
    if (!isLogicOp(opcode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->logicOp = opcode;
    ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->polygonFactor = factor;
    ctx->polygonUnits = units;
    ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((width < 0) || (height < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->scissorX = x;
    ctx->scissorY = y;
    ctx->scissorW = width;
    ctx->scissorH = height;
    ctx->flags |= GLASS_CONTEXT_FLAG_SCISSOR;
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (!isTestFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->stencilFunc = func;
    ctx->stencilRef = ref;
    ctx->stencilMask = mask;
    ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
}

void glStencilMask(GLuint mask) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->stencilWriteMask = mask;
    ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (!isStencilOp(sfail) || !isStencilOp(dpfail) || !isStencilOp(dppass)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->stencilFail = sfail;
    ctx->stencilDepthFail = dpfail;
    ctx->stencilPass = dppass;
    ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((width < 0) || (height < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->viewportX = x;
    ctx->viewportY = y;
    ctx->viewportW = width;
    ctx->viewportH = height;
    ctx->scissorMode = SCISSORMODE_DISABLE;
    ctx->flags |= (GLASS_CONTEXT_FLAG_VIEWPORT | GLASS_CONTEXT_FLAG_SCISSOR);
}