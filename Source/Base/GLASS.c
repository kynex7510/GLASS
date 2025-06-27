#include "Base/Utility.h"
#include "Base/Context.h"
#include "Platform/GPU.h"

#include <string.h> // memcpy

GLASSCtx glassCreateContext(const GLASSInitParams* initParams, const GLASSSettings* settings) {
    if (!initParams)
        return NULL;

    if (initParams->version == GLASS_VERSION_2_0) {
        CtxCommon* ctx = (CtxCommon*)glassHeapAlloc(sizeof(CtxCommon));
        if (ctx) {
            GLASS_context_initCommon(ctx, initParams, settings);
        }
        return (GLASSCtx)ctx;
    }

    return NULL;
}

GLASSCtx glassCreateDefaultContext(GLASSVersion version) {
    GLASSInitParams initParams;
    initParams.version = version;
    initParams.flushAllLinearMem = true;
    return glassCreateContextEx(&initParams, NULL);
}

void glassDestroyContext(GLASSCtx wrapped) {
    KYGX_ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)wrapped;

    if (ctx->initParams.version == GLASS_VERSION_2_0) {
        GLASS_context_cleanupCommon((CtxCommon*)ctx);
    } else {
        KYGX_UNREACHABLE("Invalid context version!");
    }

    glassVirtualFree(ctx);
}

void glassBindContext(GLASSCtx ctx) { GLASS_context_bind((CtxCommon*)ctx); }

void glassReadSettings(GLASSCtx wrapped, GLASSSettings* settings) {
    KYGX_ASSERT(wrapped);
    KYGX_ASSERT(settings);

    const CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(settings, &ctx->settings, sizeof(ctx->settings));
}

void glassWriteSettings(GLASSCtx wrapped, const GLASSSettings* settings) {
    KYGX_ASSERT(wrapped);
    KYGX_ASSERT(settings);

    CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(&ctx->settings, settings, sizeof(ctx->settings));
}

static GLenum GLASS_wrapFBFormat(GSPGPU_FramebufferFormat format) {
    switch (format) {
        case GSP_RGBA8_OES:
            return GL_RGBA8_OES;
        case GSP_BGR8_OES:
            return GL_RGB8_OES;
        case GSP_RGB565_OES:
            return GL_RGB565;
        case GSP_RGB5_A1_OES:
            return GL_RGB5_A1;
        case GSP_RGBA4_OES:
            return GL_RGBA4;
    }

    KYGX_UNREACHABLE("Invalid parameter!");
}

static void GLASS_swapScreenBuffers(CtxCommon* ctx) {
    gfxScreenSwapBuffers(ctx->settings.targetScreen, ctx->settings.targetScreen == GFX_TOP && ctx->settings.targetSide == GFX_RIGHT);
}

static void GLASS_displayTransferDone(gxCmdQueue_s* queue) {
    CtxCommon* ctx = (CtxCommon*)queue->user;
    GLASS_swapScreenBuffers(ctx);
    gxCmdQueueSetCallback(queue, NULL, NULL);
}

static void GLASS_swapBuffers(CtxCommon* ctx, bool vsync) {
    // Flush GPU commands.
    GLASS_context_flush(ctx, true);

    // Framebuffer might not be set.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer)) {
        GLASS_swapScreenBuffers(ctx);
        return;
    }

    // Get color buffer.
    if (ctx->framebuffer == GLASS_INVALID_OBJECT) {
        GLASS_swapScreenBuffers(ctx);
        return;
    }

    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
    if (fb->colorBuffer == GLASS_INVALID_OBJECT) {
        GLASS_swapScreenBuffers(ctx);
        return;
    }

    const RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
    glassPixelFormat colorPixelFormat;
    colorPixelFormat.format = cb->format;
    colorPixelFormat.type = GL_RENDERBUFFER;

    // Get display buffer.
    glassPixelFormat displayPixelFormat;
    displayPixelFormat.format = GLASS_gfx_getScreenFormat(ctx->settings.targetScreen);
    displayPixelFormat.type = GL_RENDERBUFFER;

    uint16_t displayWidth = 0;
    uint16_t displayHeight = 0;
    const void* displayBuffer = GLASS_gfx_getFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, &displayWidth, &displayHeight);

    // Perform transfer.
    GXCmd cmd;

    uint32_t transferFlags = GLASS_GX_TRANSFER_SRCFMT(GLASS_pixels_unwrapTransferFormatFlag(&colorPixelFormat));
    transferFlags |= GLASS_GX_TRANSFER_DSTFMT(GLASS_pixels_unwrapTransferFormatFlag(&displayPixelFormat));
    
    if (ctx->settings.verticalFlip)
        transferFlags |= GLASS_GX_TRANSFER_FLAG_VERTICAL_FLIP;

    GLASS_gx_makeDisplayTransfer(&cmd, cb->address, cb->height, cb->width, displayBuffer, displayWidth, displayHeight, transferFlags);
    
    if (vsync) {
        GLASS_gx_runSync(&cmd);
    } else {
        GLASS_gx_runAsync(&cmd, GLASS_asyncSwapScreenBuffers, ctx, NULL);
    }
}

void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom) {
    CtxCommon* topCtx = (CtxCommon*)top;
    CtxCommon* bottomCtx = (CtxCommon*)bottom;

    if (!topCtx && !bottomCtx)
        return;

    const bool vsync = (topCtx && topCtx->settings.vsync) || (bottomCtx && bottomCtx->settings.vsync);

    GLASS_swapBuffers(topCtx, vsync);
    GLASS_swapBuffers(bottomCtx, vsync);

    if (vsync)
        GLASS_gfx_waitForVBlank();
}

void glassSwapBuffers(void) { glassSwapContextBuffers((GLASSCtx)GLASS_context_getCommon(), NULL); }