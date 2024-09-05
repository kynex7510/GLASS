#include "Context.h"
#include "GPU.h"
#include "Utility.h"

#define REMOVE_CLEAR_BITS(mask) \
  (((((mask) & ~GL_COLOR_BUFFER_BIT) & ~GL_DEPTH_BUFFER_BIT) & ~GL_STENCIL_BUFFER_BIT) & ~GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define HAS_COLOR(mask) ((mask) & GL_COLOR_BUFFER_BIT)
#define HAS_DEPTH(mask) ((mask) & GL_DEPTH_BUFFER_BIT)
#define HAS_STENCIL(mask) ((mask) & GL_STENCIL_BUFFER_BIT)
#define HAS_EARLY_DEPTH(mask) ((mask) & GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define IS_DRAW_MODE(mode) \
    (((mode) == GL_TRIANGLES) || ((mode) == GL_TRIANGLE_STRIP) || ((mode) == GL_TRIANGLE_FAN) || ((mode) == GL_GEOMETRY_PRIMITIVE_PICA))

extern GLenum glCheckFramebufferStatus(GLenum target);

static bool GLASS_checkFB(void) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GLASS_context_setError(GL_INVALID_FRAMEBUFFER_OPERATION);
        return false;
    }

    return true;
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

    if (HAS_COLOR(mask) || HAS_DEPTH(mask)) {
        // Early depth clear is a GPU command, and its execution does not need
        // to be enforced before a clear call is issued. This doesn't apply to
        // other buffers which rely on a GX call.
        GLASS_context_update();
        GLASS_gpu_flushCommands(ctx);

        // Clear framebuffer.
        FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
        RenderbufferInfo* colorBuffer = HAS_COLOR(mask) ? fb->colorBuffer : NULL;
        const u32 clearColor = colorBuffer ? GLASS_utility_makeClearColor(colorBuffer->format, ctx->clearColor) : 0;
        RenderbufferInfo* depthBuffer = HAS_DEPTH(mask) ? fb->depthBuffer : NULL;
        const u32 clearDepth = depthBuffer ? GLASS_utility_makeClearDepth(depthBuffer->format, ctx->clearDepth, ctx->clearStencil) : 0;
        GLASS_gpu_clearBuffers(colorBuffer, clearColor, depthBuffer, clearDepth);
    }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearColor = (u32)(0xFF * CLAMP_FLOAT(red)) << 24;
    ctx->clearColor |= (u32)(0xFF * CLAMP_FLOAT(green)) << 16;
    ctx->clearColor |= (u32)(0xFF * CLAMP_FLOAT(blue)) << 8;
    ctx->clearColor |= (u32)(0xFF * CLAMP_FLOAT(alpha));
}

void glClearDepthf(GLclampf depth) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearDepth = CLAMP_FLOAT(depth);
}

void glClearStencil(GLint s) {
    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->clearStencil = s;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!IS_DRAW_MODE(mode)) {
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
    GLASS_context_update();

    // Add draw command.
    CtxCommon* ctx = GLASS_context_getCommon();
    GLASS_gpu_enableCommands(ctx);
    GLASS_gpu_drawArrays(mode, first, count);
    GLASS_gpu_disableCommands(ctx);
    ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    // TODO: use element array buffer if indices == NULL.
    ASSERT(indices);

    if (!IS_DRAW_MODE(mode)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT) {
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
    GLASS_context_update();

    // Add draw command.
    CtxCommon* ctx = GLASS_context_getCommon();
    GLASS_gpu_enableCommands(ctx);
    GLASS_gpu_drawElements(mode, count, type, indices);
    GLASS_gpu_disableCommands(ctx);
    ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glFinish(void) {
    GLASS_context_update();
    GLASS_gpu_flushAndRunCommands(GLASS_context_getCommon());
}

void glFlush(void) {
    GLASS_context_update();
    GLASS_gpu_flushCommands(GLASS_context_getCommon());
}