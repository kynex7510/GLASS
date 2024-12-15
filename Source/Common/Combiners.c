#include "Base/Context.h"

// TODO: GL_TEXTURE3 is a valid source, but that's for proctex, not a "real" texture.
static bool GLASS_isCombinerSrc(GLenum src) {
    switch (src) {
        case GL_PRIMARY_COLOR:
        case GL_FRAGMENT_PRIMARY_COLOR_PICA:
        case GL_FRAGMENT_SECONDARY_COLOR_PICA:
        case GL_TEXTURE0:
        case GL_TEXTURE1:
        case GL_TEXTURE2:
        case GL_PREVIOUS_BUFFER_PICA:
        case GL_CONSTANT:
        case GL_PREVIOUS:
            return true;
    }

    return false;
}

static bool GLASS_isCombinerOpAlpha(GLenum op) {
    switch (op) {
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_SRC_R_PICA:
        case GL_ONE_MINUS_SRC_R_PICA:
        case GL_SRC_G_PICA:
        case GL_ONE_MINUS_SRC_G_PICA:
        case GL_SRC_B_PICA:
        case GL_ONE_MINUS_SRC_B_PICA:
            return true;
    }

    return false;
}

static bool GLASS_isCombinerOpRGB(GLenum op) {
    if (GLASS_isCombinerOpAlpha(op))
        return true;

    switch (op) {
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
            return true;
    }

    return false;
}

static bool GLASS_isCombinerFunc(GLenum func) {
    switch (func) {
        case GL_REPLACE:
        case GL_MODULATE:
        case GL_ADD:
        case GL_ADD_SIGNED:
        case GL_INTERPOLATE:
        case GL_SUBTRACT:
        case GL_DOT3_RGB:
        case GL_DOT3_RGBA:
        case GL_MULT_ADD_PICA:
        case GL_ADD_MULT_PICA:
            return true;
    }

    return false;
}

static bool GLASS_isCombinerScale(GLfloat scale) { return (scale == 1.0f || scale == 2.0f || scale == 4.0f); }

void glCombinerStagePICA(GLint index) {
    if ((index < 0) || (index >= GLASS_NUM_COMBINER_STAGES)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    GLASS_context_getCommon()->combinerStage = index;
}

void glCombinerSrcPICA(GLenum pname, GLenum src) {
    if (!GLASS_isCombinerSrc(src)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    CombinerInfo* combiner = &ctx->combiners[ctx->combinerStage];
    GLenum* combinerSrc = NULL;

    switch (pname) {
        case GL_SRC0_RGB:
            combinerSrc = &combiner->rgbSrc[0];
            break;
        case GL_SRC1_RGB:
            combinerSrc = &combiner->rgbSrc[1];
            break;
        case GL_SRC2_RGB:
            combinerSrc = &combiner->rgbSrc[2];
            break;
        case GL_SRC0_ALPHA:
            combinerSrc = &combiner->alphaSrc[0];
            break;
        case GL_SRC1_ALPHA:
            combinerSrc = &combiner->alphaSrc[1];
            break;
        case GL_SRC2_ALPHA:
            combinerSrc = &combiner->alphaSrc[2];
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    ASSERT(combinerSrc);

    if (*combinerSrc != src) {
        *combinerSrc = src;
        ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
    }
}

void glCombinerColorPICA(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();
    CombinerInfo* combiner = &ctx->combiners[ctx->combinerStage];

    u32 color = (u32)(0xFF * CLAMP(0.0f, 1.0f, red));
    color |= (u32)(0xFF * CLAMP(0.0f, 1.0f, green)) << 8;
    color |= (u32)(0xFF * CLAMP(0.0f, 1.0f, blue)) << 16;
    color |= (u32)(0xFF * CLAMP(0.0f, 1.0f, alpha)) << 24;

    if (combiner->color != color) {
        combiner->color = color;
        ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
    }
}

void glCombinerOpPICA(GLenum pname, GLenum op) {
    CtxCommon* ctx = GLASS_context_getCommon();
    CombinerInfo* combiner = &ctx->combiners[ctx->combinerStage];
    GLenum* combinerOp = NULL;
    bool isRGB = false;

    switch (pname) {
        case GL_OPERAND0_RGB:
            combinerOp = &combiner->rgbOp[0];
            isRGB = true;
            break;
        case GL_OPERAND1_RGB:
            combinerOp = &combiner->rgbOp[1];
            isRGB = true;
            break;
        case GL_OPERAND2_RGB:
            combinerOp = &combiner->rgbOp[2];
            isRGB = true;
            break;
        case GL_OPERAND0_ALPHA:
            combinerOp = &combiner->alphaOp[0];
            break;
        case GL_OPERAND1_ALPHA:
            combinerOp = &combiner->alphaOp[1];
            break;
        case GL_OPERAND2_ALPHA:
            combinerOp = &combiner->alphaOp[2];
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    if (!(isRGB ? GLASS_isCombinerOpRGB(op) : GLASS_isCombinerOpAlpha(op))) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    ASSERT(combinerOp);

    if (*combinerOp != op) {
        *combinerOp = op;
        ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
    }
}

void glCombinerFuncPICA(GLenum pname, GLenum func) {
    if (!GLASS_isCombinerFunc(func)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    CombinerInfo* combiner = &ctx->combiners[ctx->combinerStage];
    GLenum* combinerFunc = NULL;

    switch (pname) {
        case GL_COMBINE_RGB:
            combinerFunc = &combiner->rgbFunc;
            break;
        case GL_COMBINE_ALPHA:
            combinerFunc = &combiner->alphaFunc;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    ASSERT(combinerFunc);

    if (*combinerFunc != func) {
        *combinerFunc = func;
        ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
    }
}

void glCombinerScalePICA(GLenum pname, GLfloat scale) {
    if (!GLASS_isCombinerScale(scale)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    CombinerInfo* combiner = &ctx->combiners[ctx->combinerStage];
    GLfloat* combinerScale = NULL;

    switch (pname) {
        case GL_RGB_SCALE:
            combinerScale = &combiner->rgbScale;
            break;
        case GL_ALPHA_SCALE:
            combinerScale = &combiner->alphaScale;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    ASSERT(combinerScale);

    if (*combinerScale != scale) {
        *combinerScale = scale;
        ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
    }
}