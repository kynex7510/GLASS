/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Utility.h>

#include "Base/Context.h"
#include "Base/TexManager.h"

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    KYGX_ASSERT(GLASS_OBJ_IS_FRAMEBUFFER(framebuffer) || framebuffer == GLASS_INVALID_OBJECT);

    if (target != GL_FRAMEBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    FramebufferInfo* info = (FramebufferInfo*)framebuffer;
    CtxCommon* ctx = GLASS_context_getBound();
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);

    // Bind framebuffer to context.
    if (ctx->framebuffer[fbIndex] != framebuffer) {
        ctx->framebuffer[fbIndex] = framebuffer;

        if (info)
            info->bound = true;

        ctx->flags |= GLASS_CONTEXT_FLAG_FRAMEBUFFER;
    }
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    KYGX_ASSERT(GLASS_OBJ_IS_RENDERBUFFER(renderbuffer) || renderbuffer == GLASS_INVALID_OBJECT);

    if (target != GL_RENDERBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    RenderbufferInfo* info = (RenderbufferInfo*)renderbuffer;
    CtxCommon* ctx = GLASS_context_getBound();

    // Bind renderbuffer to context.
    if (ctx->renderbuffer != renderbuffer) {
        ctx->renderbuffer = renderbuffer;

        if (info)
            info->bound = true;
    }
}

GLenum glCheckFramebufferStatus(GLenum target) {
    if (target != GL_FRAMEBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return 0;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);

    // Make sure we have a framebuffer.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex]))
        return GL_FRAMEBUFFER_UNSUPPORTED;

    FramebufferInfo* info = (FramebufferInfo*)ctx->framebuffer[fbIndex];

    // Check that we have at least one attachment.
    if ((info->colorBuffer == GLASS_INVALID_OBJECT) && (info->depthBuffer == GLASS_INVALID_OBJECT))
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

    // Check buffers.
    u16 cbWidth = 0;
    u16 cbHeight = 0;

    if (GLASS_OBJ_IS_RENDERBUFFER(info->colorBuffer)) {
        const RenderbufferInfo* cb = (RenderbufferInfo*)info->colorBuffer;

        if (!cb->address)
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;

        cbWidth = cb->width;
        cbHeight = cb->height;
    } else if (GLASS_OBJ_IS_TEXTURE(info->colorBuffer)) {
        RenderbufferInfo cb;
        GLASS_tex_getAsRenderbuffer((const TextureInfo*)info->colorBuffer, info->texFace, &cb);

        if (!cb.address)
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;

        cbWidth = cb.width;
        cbHeight = cb.height;
    }
    
    const RenderbufferInfo* db = (RenderbufferInfo*)info->depthBuffer;

    if (db && !db->address)
        return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;

    if (info->colorBuffer != GLASS_INVALID_OBJECT && db && ((cbWidth != db->width) || (cbHeight != db->height)))
        return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;

    return GL_FRAMEBUFFER_COMPLETE;
}

void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    KYGX_ASSERT(framebuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();

    for (size_t i = 0; i < n; ++i) {
        GLuint name = framebuffers[i];

        // Validate name.
        if (!GLASS_OBJ_IS_FRAMEBUFFER(name))
            continue;

        // Unbind if bound.
        for (size_t i = 0; i < 2; ++i) {
        if (ctx->framebuffer[i] == name)
            ctx->framebuffer[i] = GLASS_INVALID_OBJECT;
        }

        // Delete framebuffer.
        glassHeapFree((FramebufferInfo*)name);
    }
}

void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    KYGX_ASSERT(renderbuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);

    // Get framebuffer.
    FramebufferInfo* fbInfo = NULL;
    if (GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex]))
        fbInfo = (FramebufferInfo*)ctx->framebuffer[fbIndex];

    for (size_t i = 0; i < n; ++i) {
        GLuint name = renderbuffers[i];

        // Validate name.
        if (!GLASS_OBJ_IS_RENDERBUFFER(name))
            continue;

        RenderbufferInfo* info = (RenderbufferInfo*)name;

        // Unbind if bound.
        if (fbInfo) {
            if (fbInfo->colorBuffer == name) {
                fbInfo->colorBuffer = GLASS_INVALID_OBJECT;
            } else if (fbInfo->depthBuffer == name) {
                fbInfo->depthBuffer = GLASS_INVALID_OBJECT;
            }
        }

        // Delete renderbuffer.
        if (info->address)
            glassVRAMFree(info->address);

        glassHeapFree(info);
    }
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    if ((target != GL_FRAMEBUFFER) || (renderbuffertarget != GL_RENDERBUFFER)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (!GLASS_OBJ_IS_RENDERBUFFER(renderbuffer) && renderbuffer != GLASS_INVALID_OBJECT) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);

    // Get framebuffer.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex])) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    FramebufferInfo* fbInfo = (FramebufferInfo*)ctx->framebuffer[fbIndex];

    // Set right buffer.
    switch (attachment) {
        case GL_COLOR_ATTACHMENT0:
            fbInfo->colorBuffer = renderbuffer;
            break;
        case GL_DEPTH_ATTACHMENT:
        case GL_STENCIL_ATTACHMENT:
            fbInfo->depthBuffer = renderbuffer;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    ctx->flags |= GLASS_CONTEXT_FLAG_FRAMEBUFFER;
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    if (target != GL_FRAMEBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (attachment != GL_COLOR_ATTACHMENT0) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    // Get the framebuffer.
    CtxCommon* ctx = GLASS_context_getBound();
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);

    if (ctx->framebuffer[fbIndex] == GLASS_INVALID_OBJECT) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    FramebufferInfo* fbInfo = (FramebufferInfo*)ctx->framebuffer[fbIndex];

    // Handle the case where we need to remove the binding.
    if (texture == GLASS_INVALID_OBJECT) {
        fbInfo->colorBuffer = GLASS_INVALID_OBJECT;
        return;
    }

    // Do additional checks.
    if (!GLASS_OBJ_IS_TEXTURE(texture)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (level != 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    size_t face = -1;
    bool cubemapTarget = true;

    switch (textarget) {
        case GL_TEXTURE_2D:
            face = 0;
            cubemapTarget = false;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            face = 0;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            face = 1;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            face = 2;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            face = 3;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            face = 4;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            face = 5;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    TextureInfo* tex = (TextureInfo*)texture;
    
    if (cubemapTarget && tex->target != GL_TEXTURE_CUBE_MAP) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (!cubemapTarget && tex->target != GL_TEXTURE_2D) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Set texture object.
    fbInfo->colorBuffer = texture;
    fbInfo->texFace = face;
    ctx->flags |= GLASS_CONTEXT_FLAG_FRAMEBUFFER;
}

void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    KYGX_ASSERT(framebuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_FRAMEBUFFER_TYPE);
        if (!GLASS_OBJ_IS_FRAMEBUFFER(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        framebuffers[i] = name;
    }
}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    KYGX_ASSERT(renderbuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_RENDERBUFFER_TYPE);
        if (!GLASS_OBJ_IS_RENDERBUFFER(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        RenderbufferInfo* info = (RenderbufferInfo*)name;
        info->format = GL_RGBA8_OES;
        renderbuffers[i] = name;
    }
}

static inline GLint getColorSize(GLenum format, GLenum color) {
    switch (format) {
        case GL_RGBA8_OES:
            return 8;
        case GL_RGB5_A1:
            return color == GL_RENDERBUFFER_ALPHA_SIZE ? 1 : 5;
        case GL_RGB565:
            if (color == GL_RENDERBUFFER_GREEN_SIZE) {
                return 6;
            } else if (color == GL_RENDERBUFFER_ALPHA_SIZE) {
                return 0;
            } else {
                return 5;
            }
        case GL_RGBA4:
            return 4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GLint getDepthSize(GLenum format) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
            return 16;
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 24;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    KYGX_ASSERT(params);

    if (target != GL_RENDERBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();

    // Get renderbuffer.
    if (!GLASS_OBJ_IS_RENDERBUFFER(ctx->renderbuffer)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    RenderbufferInfo* info = (RenderbufferInfo*)ctx->renderbuffer;

    switch (pname) {
        case GL_RENDERBUFFER_WIDTH:
            *params = info->width;
            break;
        case GL_RENDERBUFFER_HEIGHT:
            *params = info->height;
            break;
        case GL_RENDERBUFFER_INTERNAL_FORMAT:
            *params = info->format;
            break;
        case GL_RENDERBUFFER_RED_SIZE:
        case GL_RENDERBUFFER_GREEN_SIZE:
        case GL_RENDERBUFFER_BLUE_SIZE:
        case GL_RENDERBUFFER_ALPHA_SIZE:
            *params = getColorSize(info->format, pname);
            break;
        case GL_RENDERBUFFER_DEPTH_SIZE:
            *params = getDepthSize(info->format);
            break;
        case GL_RENDERBUFFER_STENCIL_SIZE:
            *params = info->format == GL_DEPTH24_STENCIL8_OES ? 8 : 0;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }
}

GLboolean glIsFramebuffer(GLuint framebuffer) {
    if (GLASS_OBJ_IS_FRAMEBUFFER(framebuffer)) {
        FramebufferInfo* info = (FramebufferInfo*)framebuffer;
        if (info->bound)
            return GL_TRUE;
    }

    return GL_FALSE;
}

GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    if (GLASS_OBJ_IS_RENDERBUFFER(renderbuffer)) {
        RenderbufferInfo* info = (RenderbufferInfo*)renderbuffer;
        if (info->bound)
            return GL_TRUE;
    }

    return GL_FALSE;
}

static inline bool isColorFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
            return true;
    }

    return false;
}

static inline bool isDepthFormat(GLenum format) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return true;
    }

    return false;
}

static inline size_t getBytesPerPixel(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 4;
        case GL_DEPTH_COMPONENT24_OES:
            return 3;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
        case GL_DEPTH_COMPONENT16:
            return 2;
        default:
            KYGX_UNREACHABLE("Invalid format!");
    }

    return 0;
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    if ((target != GL_RENDERBUFFER) || (!isColorFormat(internalformat) && !isDepthFormat(internalformat))) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if ((width <= 0) || (height <= 0) || (width > GLASS_MAX_FB_WIDTH) || (height > GLASS_MAX_FB_HEIGHT)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Renderbuffer dimensions must be multiple of 8.
    if (!kygxIsAligned(width, 8) || !kygxIsAligned(height, 8)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();

    // Get renderbuffer.
    if (!GLASS_OBJ_IS_RENDERBUFFER(ctx->renderbuffer)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    RenderbufferInfo* info = (RenderbufferInfo* )ctx->renderbuffer;

    // Allocate buffer.
    const size_t bufferSize = width * height * getBytesPerPixel(internalformat);

    if (info->address)
        glassVRAMFree(info->address);

    const bool isDepth = isDepthFormat(internalformat);
    info->address = glassVRAMAlloc(bufferSize, isDepth ? KYGX_ALLOC_VRAM_BANK_B : KYGX_ALLOC_VRAM_BANK_A);

    if (!info->address)
        info->address = glassVRAMAlloc(bufferSize, isDepth ? KYGX_ALLOC_VRAM_BANK_A : KYGX_ALLOC_VRAM_BANK_B);

    if (!info->address) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        return;
    }

    info->width = width;
    info->height = height;
    info->format = internalformat;
}