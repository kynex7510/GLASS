#include "Base/Context.h"
#include "Base/GPU.h"
#include "Base/GX.h"
#include "Base/Utility.h"

#define REMOVE_CLEAR_BITS(mask) \
  (((((mask) & ~GL_COLOR_BUFFER_BIT) & ~GL_DEPTH_BUFFER_BIT) & ~GL_STENCIL_BUFFER_BIT) & ~GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define HAS_COLOR(mask) ((mask) & GL_COLOR_BUFFER_BIT)
#define HAS_DEPTH(mask) ((mask) & GL_DEPTH_BUFFER_BIT)
#define HAS_STENCIL(mask) ((mask) & GL_STENCIL_BUFFER_BIT)
#define HAS_EARLY_DEPTH(mask) ((mask) & GL_EARLY_DEPTH_BUFFER_BIT_PICA)

static bool GLASS_checkFB(void) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GLASS_context_setError(GL_INVALID_FRAMEBUFFER_OPERATION);
        return false;
    }

    return true;
}

static u32 GLASS_makeClearColor(GLenum format, u32 color) {
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
            UNREACHABLE("Invalid color format!");
    }

    return cvt;
}

static u32 GLASS_makeClearDepth(GLenum format, GLclampf factor, u8 stencil) {
    ASSERT(factor >= 0.0 && factor <= 1.0);

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
            UNREACHABLE("Invalid depth format!");
    }

    return clearDepth;
}

void glClear(GLbitfield mask) {
    // Check parameters.
    if (REMOVE_CLEAR_BITS(mask) || (!HAS_DEPTH(mask) && HAS_STENCIL(mask))) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!GLASS_checkFB())
        return;

    CtxCommon* ctx = GLASS_context_getCommon();

    // Clear early depth buffer.
    if (HAS_EARLY_DEPTH(mask))
        ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH_CLEAR;

    // Clear framebuffers.
    if (HAS_COLOR(mask) || HAS_DEPTH(mask)) {
        FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
        RenderbufferInfo* colorBuffer = HAS_COLOR(mask) ? fb->colorBuffer : NULL;
        const u32 clearColor = colorBuffer ? GLASS_makeClearColor(colorBuffer->format, ctx->clearColor) : 0;
        RenderbufferInfo* depthBuffer = HAS_DEPTH(mask) ? fb->depthBuffer : NULL;
        const u32 clearDepth = depthBuffer ? GLASS_makeClearDepth(depthBuffer->format, ctx->clearDepth, ctx->clearStencil) : 0;
        GLASS_gx_clearBuffers(colorBuffer, clearColor, depthBuffer, clearDepth);
    }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearColor = (u32)(0xFF * CLAMP(0.0f, 1.0f, red)) << 24;
    ctx->clearColor |= (u32)(0xFF * CLAMP(0.0f, 1.0f, green)) << 16;
    ctx->clearColor |= (u32)(0xFF * CLAMP(0.0f, 1.0f, blue)) << 8;
    ctx->clearColor |= (u32)(0xFF * CLAMP(0.0f, 1.0f, alpha));
}

void glClearDepthf(GLclampf depth) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearDepth = CLAMP(0.0f, 1.0f, depth);
}

void glClearStencil(GLint s) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearStencil = s;
}

static bool GLASS_isDrawMode(GLenum mode) {
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
    if (!GLASS_isDrawMode(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (count < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!GLASS_checkFB())
        return;

    // Apply prior commands.
    GLASS_context_flush();

    // Add draw command.
    CtxCommon* ctx = GLASS_context_getCommon();
    GLASS_gpu_enableCommands();
    GLASS_gpu_drawArrays(mode, first, count);
    GLASS_gpu_disableCommands();
    ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    if (!GLASS_isDrawMode(mode)) {
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

    if (!GLASS_checkFB())
        return;

    // Get physical address.
    CtxCommon* ctx = GLASS_context_getCommon();
    u32 physAddr = 0;
    if (ctx->elementArrayBuffer != GLASS_INVALID_OBJECT) {
        const BufferInfo* binfo = (BufferInfo*)ctx->elementArrayBuffer;
        physAddr = osConvertVirtToPhys((void*)(binfo->address + (u32)indices));
    } else {
        physAddr = osConvertVirtToPhys(indices);
    }

    ASSERT(physAddr);

    // Apply prior commands.
    GLASS_context_flush();

    // Add draw command.
    GLASS_gpu_enableCommands();
    GLASS_gpu_drawElements(mode, count, type, physAddr);
    GLASS_gpu_disableCommands();
    ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glFlush(void) { GLASS_context_flush(); }

void glFinish(void) {
    GLASS_context_flush();
    GLASS_gx_sendGPUCommands();
}