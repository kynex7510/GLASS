#include <KYGX/Wrappers/MemoryFill.h>
#include <KYGX/Utility.h>

#include "Base/Context.h"
#include "Base/Math.h"
#include "Platform/GPU.h"

#include <string.h> // memset

#define REMOVE_CLEAR_BITS(mask) \
  (((((mask) & ~GL_COLOR_BUFFER_BIT) & ~GL_DEPTH_BUFFER_BIT) & ~GL_STENCIL_BUFFER_BIT) & ~GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define HAS_COLOR(mask) ((mask) & GL_COLOR_BUFFER_BIT)
#define HAS_DEPTH(mask) ((mask) & GL_DEPTH_BUFFER_BIT)
#define HAS_STENCIL(mask) ((mask) & GL_STENCIL_BUFFER_BIT)
#define HAS_EARLY_DEPTH(mask) ((mask) & GL_EARLY_DEPTH_BUFFER_BIT_PICA)

// Defined in Framebuffer.c.
extern GLenum glCheckFramebufferStatus(GLenum target);

static inline bool checkFB(void) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GLASS_context_setError(GL_INVALID_FRAMEBUFFER_OPERATION);
        return false;
    }

    return true;
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

static inline u32 makeClearColor(GLenum format, u32 color) {
    u32 cvt = 0;

    switch (format) {
        case GL_RGBA8_OES:
            cvt = color;
            break;
        case GL_RGB8_OES:
            cvt = color >> 8;
            break;
        case GL_RGBA4:
            cvt = ((color >> 24) & 0x0F) << 12;
            cvt |= ((color >> 16) & 0x0F) << 8;
            cvt |= ((color >> 8) & 0x0F) << 4;
            cvt |= color & 0x0F;
            break;
        case GL_RGB5_A1:
            cvt = ((color >> 24) & 0x1F) << 11;
            cvt |= ((color >> 16) & 0x1F) << 6;
            cvt |= ((color >> 8) & 0x1F) << 1;
            cvt |= (color & 0xFF) != 0;
            break;
        case GL_RGB565:
            cvt = ((color >> 24) & 0x1F) << 11;
            cvt |= ((color >> 16) & 0x3F) << 5;
            cvt |= (color >> 8) & 0x1F;
            break;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return cvt;
}

static u32 makeClearDepth(GLenum format, GLclampf factor, u8 stencil) {
    KYGX_ASSERT(factor >= 0.0 && factor <= 1.0);

    u32 clearDepth = 0;
    switch (format) {
        case GL_DEPTH_COMPONENT16:
            clearDepth = (u32)(0xFFFF * factor);
            break;
        case GL_DEPTH_COMPONENT24_OES:
            clearDepth = (u32)(0xFFFFFF * factor);
            break;
        case GL_DEPTH24_STENCIL8_OES:
            clearDepth = (((u32)(0xFFFFFF * factor) << 8) | stencil);
            break;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return clearDepth;
}

static inline u8 fillWidth(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return KYGX_MEMORYFILL_WIDTH_32;
        case GL_RGB8_OES:
        case GL_DEPTH_COMPONENT24_OES:
            return KYGX_MEMORYFILL_WIDTH_24;
        case GL_RGB565:
        case GL_RGB5_A1:
        case GL_RGBA4:
        case GL_DEPTH_COMPONENT16:
            return KYGX_MEMORYFILL_WIDTH_16;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void glClear(GLbitfield mask) {
    // Check parameters.
    if (REMOVE_CLEAR_BITS(mask) || (!HAS_DEPTH(mask) && HAS_STENCIL(mask))) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!checkFB())
        return;

    CtxCommon* ctx = GLASS_context_getBound();

    // Clear early depth buffer.
    if (HAS_EARLY_DEPTH(mask))
        ctx->flags |= GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR;

    // Clear framebuffers.
    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;

    KYGXMemoryFillBuffer colorFill;
    memset(&colorFill, 0, sizeof(KYGXMemoryFillBuffer));

    if (HAS_COLOR(mask)) {
        const RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
        if (cb) {
            colorFill.addr = cb->address;
            colorFill.size = cb->width * cb->height * getBytesPerPixel(cb->format);
            colorFill.value = makeClearColor(cb->format, ctx->clearColor);
            colorFill.width = fillWidth(cb->format);
        }
    }
    
    KYGXMemoryFillBuffer depthFill;
    memset(&depthFill, 0, sizeof(KYGXMemoryFillBuffer));

    if (HAS_DEPTH(mask)) {
        const RenderbufferInfo* db = (RenderbufferInfo*)fb->depthBuffer;
        if (db) {
            depthFill.addr = db->address;
            depthFill.size = db->width * db->height * getBytesPerPixel(db->format);
            depthFill.value = makeClearDepth(db->format, ctx->clearDepth, ctx->clearStencil);
            depthFill.width = fillWidth(db->format);
        }
    }

    if (colorFill.addr || depthFill.addr) {
        // Flush GPU commands to enforce draw order.
        GLASS_context_flush(ctx, true);

        kygxLock();
        kygxAddMemoryFill(&ctx->GXCmdBuf, &colorFill, &depthFill);
        kygxCmdBufferFinalize(&ctx->GXCmdBuf, NULL, NULL);
        kygxUnlock(true);
    }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->clearColor = (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, red)) << 24;
    ctx->clearColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, green)) << 16;
    ctx->clearColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, blue)) << 8;
    ctx->clearColor |= (u32)(0xFF * GLASS_CLAMP(0.0f, 1.0f, alpha));
}

void glClearDepthf(GLclampf depth) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->clearDepth = GLASS_CLAMP(0.0f, 1.0f, depth);
}

void glClearStencil(GLint s) {
    CtxCommon* ctx = GLASS_context_getBound();
    ctx->clearStencil = s;
}

static inline bool isDrawMode(GLenum mode) {
    switch (mode) {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_GEOMETRY_PRIMITIVE_PICA:
            return true;
    }

    return false;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!isDrawMode(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (count < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!checkFB())
        return;

    // Apply prior commands.
    CtxCommon* ctx = GLASS_context_getBound();
    GLASS_context_flush(ctx, false);

    // Add draw command.
    GLASS_gpu_drawArrays(&ctx->settings.GPUCmdList, mode, first, count);
    ctx->flags |= GLASS_CONTEXT_FLAG_DRAW;
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    if (!isDrawMode(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if ((type != GL_UNSIGNED_BYTE) && (type != GL_UNSIGNED_SHORT)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (count < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!checkFB())
        return;

    // Get physical address.
    CtxCommon* ctx = GLASS_context_getBound();
    u32 physAddr = 0;
    if (ctx->elementArrayBuffer != GLASS_INVALID_OBJECT) {
        const BufferInfo* binfo = (BufferInfo*)ctx->elementArrayBuffer;
        physAddr = kygxGetPhysicalAddress((void*)(binfo->address + (u32)indices));
    } else {
        physAddr = kygxGetPhysicalAddress(indices);
    }

    KYGX_ASSERT(physAddr);

    // Apply prior commands.
    GLASS_context_flush(ctx, false);

    // Add draw command.
    GLASS_gpu_drawElements(&ctx->settings.GPUCmdList, mode, count, type, physAddr);
    ctx->flags |= GLASS_CONTEXT_FLAG_DRAW;
}

void glFlush(void) { GLASS_context_flush(GLASS_context_getBound(), true); }

void glFinish(void) {
    GLASS_context_flush(GLASS_context_getBound(), true);
    kygxWaitCompletion();
}