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