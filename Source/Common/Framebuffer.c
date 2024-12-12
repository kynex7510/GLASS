#include "Base/Context.h"
#include "Base/Utility.h"

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    ASSERT(OBJ_IS_FRAMEBUFFER(framebuffer) || framebuffer == GLASS_INVALID_OBJECT);

    if (target != GL_FRAMEBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    FramebufferInfo* info = (FramebufferInfo*)framebuffer;
    CtxCommon* ctx = GLASS_context_getCommon();

    // Bind framebuffer to context.
    if (ctx->framebuffer != framebuffer) {
        ctx->framebuffer = framebuffer;

        if (info)
            info->flags |= FRAMEBUFFER_FLAG_BOUND;

        ctx->flags |= CONTEXT_FLAG_FRAMEBUFFER;
    }
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    ASSERT(OBJ_IS_RENDERBUFFER(renderbuffer) || renderbuffer == GLASS_INVALID_OBJECT);

    if (target != GL_RENDERBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    RenderbufferInfo* info = (RenderbufferInfo*)renderbuffer;
    CtxCommon* ctx = GLASS_context_getCommon();

    // Bind renderbuffer to context.
    if (ctx->renderbuffer != renderbuffer) {
        ctx->renderbuffer = renderbuffer;

        if (info)
            info->flags |= RENDERBUFFER_FLAG_BOUND;
    }
}

GLenum glCheckFramebufferStatus(GLenum target) {
    if (target != GL_FRAMEBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return 0;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    // Make sure we have a framebuffer.
    if (!OBJ_IS_FRAMEBUFFER(ctx->framebuffer))
        return GL_FRAMEBUFFER_UNSUPPORTED;

    FramebufferInfo* info = (FramebufferInfo*)ctx->framebuffer;

    // Check that we have at least one attachment.
    if (!info->colorBuffer && !info->depthBuffer)
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

    // Check buffers.
    if (info->colorBuffer) {
        if (!info->colorBuffer->address)
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    }

    if (info->depthBuffer) {
        if (!info->depthBuffer->address)
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    }

    if (info->colorBuffer && info->depthBuffer) {
        if ((info->colorBuffer->width != info->depthBuffer->width) || (info->colorBuffer->height != info->depthBuffer->height))
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    ASSERT(framebuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    for (size_t i = 0; i < n; ++i) {
        GLuint name = framebuffers[i];

        // Validate name.
        if (!OBJ_IS_FRAMEBUFFER(name))
            continue;

        // Unbind if bound.
        if (ctx->framebuffer == name)
            ctx->framebuffer = GLASS_INVALID_OBJECT;

        // Delete framebuffer.
        glassVirtualFree((FramebufferInfo*)name);
    }
}

void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    ASSERT(renderbuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    // Get framebuffer.
    FramebufferInfo* fbinfo = NULL;
    if (OBJ_IS_FRAMEBUFFER(ctx->framebuffer))
        fbinfo = (FramebufferInfo*)ctx->framebuffer;

    for (size_t i = 0; i < n; ++i) {
        GLuint name = renderbuffers[i];

        // Validate name.
        if (!OBJ_IS_RENDERBUFFER(name))
            continue;

        RenderbufferInfo* info = (RenderbufferInfo*)name;

        // Unbind if bound.
        if (fbinfo) {
            if (fbinfo->colorBuffer == info) {
                fbinfo->colorBuffer = NULL;
            } else if (fbinfo->depthBuffer == info) {
                fbinfo->depthBuffer = NULL;
            }
        }

        // Delete renderbuffer.
        if (info->address)
            glassVRAMFree(info->address);

        glassVirtualFree(info);
    }
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    if ((target != GL_FRAMEBUFFER) || (renderbuffertarget != GL_RENDERBUFFER)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (!OBJ_IS_RENDERBUFFER(renderbuffer) && renderbuffer != GLASS_INVALID_OBJECT) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    RenderbufferInfo* rbinfo = (RenderbufferInfo*)renderbuffer;
    CtxCommon* ctx = GLASS_context_getCommon();

    // Get framebuffer.
    if (!OBJ_IS_FRAMEBUFFER(ctx->framebuffer)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    FramebufferInfo* fbinfo = (FramebufferInfo*)ctx->framebuffer;

    // Set right buffer.
    switch (attachment) {
        case GL_COLOR_ATTACHMENT0:
            fbinfo->colorBuffer = rbinfo;
            break;
        case GL_DEPTH_ATTACHMENT:
        case GL_STENCIL_ATTACHMENT:
            fbinfo->depthBuffer = rbinfo;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    ctx->flags |= CONTEXT_FLAG_FRAMEBUFFER;
}

void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    ASSERT(framebuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_FRAMEBUFFER_TYPE);
        if (!OBJ_IS_FRAMEBUFFER(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        framebuffers[i] = name;
    }
}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    ASSERT(renderbuffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_RENDERBUFFER_TYPE);
        if (!OBJ_IS_RENDERBUFFER(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        RenderbufferInfo* info = (RenderbufferInfo*)name;
        info->format = GL_RGBA4;
        renderbuffers[i] = name;
    }
}

static GLint GLASS_getColorSize(GLenum format, GLenum color) {
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
    }

    UNREACHABLE("Invalid color format!");
}

static GLint GLASS_getDepthSize(GLenum format) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
            return 16;
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 24;
    }

    UNREACHABLE("Invalid depth format!");
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    ASSERT(params);

    if (target != GL_RENDERBUFFER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    // Get renderbuffer.
    if (!OBJ_IS_RENDERBUFFER(ctx->renderbuffer)) {
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
            *params = GLASS_getColorSize(info->format, pname);
            break;
        case GL_RENDERBUFFER_DEPTH_SIZE:
            *params = GLASS_getDepthSize(info->format);
            break;
        case GL_RENDERBUFFER_STENCIL_SIZE:
            *params = info->format == GL_DEPTH24_STENCIL8_OES ? 8 : 0;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
    }
}

GLboolean glIsFramebuffer(GLuint framebuffer) {
    if (OBJ_IS_FRAMEBUFFER(framebuffer)) {
        FramebufferInfo* info = (FramebufferInfo*)framebuffer;
        if (info->flags & FRAMEBUFFER_FLAG_BOUND)
            return GL_TRUE;
    }

    return GL_FALSE;
}

GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    if (OBJ_IS_RENDERBUFFER(renderbuffer)) {
        RenderbufferInfo* info = (RenderbufferInfo*)renderbuffer;
        if (info->flags & RENDERBUFFER_FLAG_BOUND)
            return GL_TRUE;
    }

    return GL_FALSE;
}

static bool GLASS_isColorFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
            return true;
    }

    return false;
}

static bool GLASS_isDepthFormat(GLenum format) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return true;
    }

    return false;
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    if ((target != GL_RENDERBUFFER) || (!GLASS_isColorFormat(internalformat) && !GLASS_isDepthFormat(internalformat))) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if ((width <= 0) || (height <= 0) || (width > GLASS_MAX_FB_WIDTH) || (height > GLASS_MAX_FB_HEIGHT)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    // Get renderbuffer.
    if (!OBJ_IS_RENDERBUFFER(ctx->renderbuffer)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    RenderbufferInfo* info = (RenderbufferInfo* )ctx->renderbuffer;

    // Allocate buffer.
    const size_t bufferSize = (width * height * GLASS_utility_getRBBytesPerPixel(internalformat));

    if (info->address)
        glassVRAMFree(info->address);

    const bool isDepth = GLASS_isDepthFormat(internalformat);
    info->address = glassVRAMAlloc(bufferSize, isDepth ? VRAM_ALLOC_B : VRAM_ALLOC_A);

    if (!info->address)
        info->address = glassVRAMAlloc(bufferSize, isDepth ? VRAM_ALLOC_A : VRAM_ALLOC_B);

    if (!info->address) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        return;
    }

    info->width = width;
    info->height = height;
    info->format = internalformat;
}