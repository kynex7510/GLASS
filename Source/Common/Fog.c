/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Base/Context.h"
#include "Base/Math.h"

void glFogPICA(GLenum pname, const GLfloat* params) {
    CtxCommon* ctx = GLASS_context_getBound();

    switch (pname) {
        case GL_FOG_COLOR:
            ctx->fogColor = (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, params[0]));
            ctx->fogColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, params[1])) << 8;
            ctx->fogColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, params[2])) << 16;
            ctx->fogColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, params[3])) << 24;
            ctx->flags |= GLASS_CONTEXT_FLAG_COMBINER_BUFFER;
            break;
        case GL_FOG_LUT_PICA:
            memcpy(ctx->fogLut, params, sizeof(ctx->fogLut));
            ctx->flags |= GLASS_CONTEXT_FLAG_FOG_LUT;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
    }
}

static inline bool isFogMode(GLenum mode) {
    switch (mode) {
        case GL_LINEAR:
        case GL_EXP:
        case GL_EXP2:
            return true;
    }

    return false;
}

GLenum glassMakeFogLut(GLenum mode, const GLfloat* projMtx, GLfloat start, GLfloat end, GLfloat density, GLfloat near, GLfloat far, GLfloat* out) {
    if (!isFogMode(mode))
        return GL_INVALID_ENUM;
    
    for (size_t i = 0; i < GLASS_FOG_LUT_SIZE; ++i) {
        const GLfloat x = GLASS_CLAMP(-1.0f, 0.0f, (near - (i / 128.0f)) / (far - near));
        const GLfloat z = ((projMtx[10] * x) - projMtx[11]) / (projMtx[15] + (projMtx[14] * x));

        GLfloat computed = 1.0f;
        if (z > start && z <= end) {
            switch (mode) {
                case GL_LINEAR:
                    computed = (end - z) / (end - start);
                    break;
                case GL_EXP:
                    computed = expf(-(density * (z - start)) / (end - start));
                    break;
                case GL_EXP2:
                    computed = expf(-((density * (z - start) / (end - start)) * (density * (z - start) / (end - start))));
                    break;
            }
        }

        out[i] = computed;
    }

    return GL_NO_ERROR;
}