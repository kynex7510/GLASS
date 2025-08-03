/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <kazmath/kazmath.h>

#include "Base/Context.h"

static inline bool isMtxMode(GLenum mode) {
    switch (mode) {
        case GL_MODELVIEW:
        case GL_PROJECTION:
        case GL_TEXTURE:
            return true;
        default:
            return false;
    }
}

static MtxStack* getCurrentMtxStack(Ctx11* ctx) {
    KYGX_ASSERT(ctx);

    if (ctx->mtxMode == GL_TEXTURE) {
        switch (ctx->common.activeTextureUnit) {
            case 0:
                return &ctx->tex0MtxStack;
            case 1:
                return &ctx->tex1MtxStack;
            case 2:
                return &ctx->tex2MtxStack;
            default:
                KYGX_UNREACHABLE("Invalid texunit!");
        }
    }

    switch (ctx->mtxMode) {
        case GL_MODELVIEW:
            return &ctx->modelViewMtxStack;
        case GL_PROJECTION:
            return &ctx->projMtxStack;
        default:
            KYGX_UNREACHABLE("Invalid matrix mode!");
    }

    return NULL;
}

void glPushMatrix(void) {
    Ctx11* ctx = GLASS_context_getBound11();
    MtxStack* s = getCurrentMtxStack(ctx);

    if (!GLASS_mtxstack_push(s)) {
        GLASS_context_setError(GL_STACK_OVERFLOW);
        return;
    }

    ctx->common.flags |= GLASS_CONTEXT_FLAG_MATRIX;
}

void glPopMatrix(void) {
    Ctx11* ctx = GLASS_context_getBound11();
    MtxStack* s = getCurrentMtxStack(ctx);

    if (!GLASS_mtxstack_pop(s)) {
        GLASS_context_setError(GL_STACK_UNDERFLOW);
        return;
    }

    ctx->common.flags |= GLASS_CONTEXT_FLAG_MATRIX;
}

void glLoadMatrixf(const GLfloat* m) {
    Ctx11* ctx = GLASS_context_getBound11();
    MtxStack* s = getCurrentMtxStack(ctx);
    GLASS_mtxstack_load(s, m);
    ctx->common.flags |= GLASS_CONTEXT_FLAG_MATRIX;
}

void glLoadIdentity(void) {
    kmMat4 tmp;
    kmMat4Identity(&tmp);
    glLoadMatrixf(tmp.mat);
}

void glMatrixMode(GLenum mode) {
    if (!isMtxMode(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    Ctx11* ctx = GLASS_context_getBound11();
    ctx->mtxMode = mode;
}

void glMultMatrixf(const GLfloat* m) {
    Ctx11* ctx = GLASS_context_getBound11();
    MtxStack* s = getCurrentMtxStack(ctx);
    GLASS_mtxstack_multiply(s, m);
    ctx->common.flags |= GLASS_CONTEXT_FLAG_MATRIX;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    kmMat4 tmp;
    kmMat4Translation(&tmp, x, y, z);
    glMultMatrixf(tmp.mat);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    kmMat4 tmp;
    kmMat4Scaling(&tmp, x, y, z);
    glMultMatrixf(tmp.mat);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    kmVec3 axis;
    kmVec3 tmpVec;
    kmVec3Fill(&tmpVec, x, y, z);
    kmVec3Normalize(&axis, &tmpVec);

    kmMat4 tmpMat;
    kmMat4RotationAxisAngle(&tmpMat, &axis, kmDegreesToRadians(angle));
    glMultMatrixf(tmpMat.mat);
}

void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
    kmMat4 tmp;
    kmMat4OrthoTilt(&tmp, l, r, b, t, n, f, false);
    glMultMatrixf(tmp.mat);
}

// TODO: verify that this is correct.
void glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
    const GLfloat width = r - l;
    const GLfloat height = t - b;
    const GLfloat aspect = width / height;
    const GLfloat fovY = 2.0f * atanf(height / (2.0f * n)) * kmPIUnder180;

    kmMat4 tmp;
    kmMat4PerspTilt(&tmp, fovY, aspect, n, f, false);
    glMultMatrixf(tmp.mat);
}