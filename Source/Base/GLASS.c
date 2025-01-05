#include "Base/Utility.h"
#include "Base/Context.h"
#include "Platform/GPU.h"
#include "Platform/GX.h"

#include <string.h> // memcpy

glassCtx glassCreateContext(glassVersion version) {
    glassInitParams initParams;
    initParams.version = version;
    initParams.flushAllLinearMem = true;
    return glassCreateContextEx(&initParams, NULL);
}

glassCtx glassCreateContextEx(const glassInitParams* initParams, const glassSettings* settings) {
    if (!initParams)
        return NULL;

    if (initParams->version == GLASS_VERSION_2_0) {
        CtxCommon* ctx = (CtxCommon*)glassVirtualAlloc(sizeof(CtxCommon));
        if (ctx) {
            GLASS_context_initCommon(ctx, initParams, settings);
        }
        return (glassCtx)ctx;
    }

    return NULL;
}

void glassDestroyContext(glassCtx wrapped) {
    ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)wrapped;

    if (ctx->initParams.version == GLASS_VERSION_2_0) {
        GLASS_context_cleanupCommon((CtxCommon*)ctx);
    } else {
        UNREACHABLE("Invalid context version!");
    }

    glassVirtualFree(ctx);
}

void glassBindContext(glassCtx ctx) { GLASS_context_bind((CtxCommon*)ctx); }

void glassReadSettings(glassCtx wrapped, glassSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

    const CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(settings, &ctx->settings, sizeof(ctx->settings));
}

void glassWriteSettings(glassCtx wrapped, const glassSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

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

    UNREACHABLE("Invalid parameter!");
}

static void GLASS_swapScreenBuffers(CtxCommon* ctx) {
    gfxScreenSwapBuffers(ctx->settings.targetScreen, ctx->settings.targetScreen == GFX_TOP && ctx->settings.targetSide == GFX_RIGHT);
}

static void GLASS_displayTransferDone(gxCmdQueue_s* queue) {
    CtxCommon* ctx = (CtxCommon*)queue->user;
    GLASS_swapScreenBuffers(ctx);
    gxCmdQueueSetCallback(queue, NULL, NULL);
}

void glassSwapBuffers(void) {
    CtxCommon* ctx = GLASS_context_getCommon();

    // Flush GPU commands.
    GLASS_context_flush();
    GLASS_gx_sendGPUCommands();

    // Framebuffer might not be set.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer))
        return;

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
    displayPixelFormat.format = GLASS_wrapFBFormat(gfxGetScreenFormat(ctx->settings.targetScreen));
    displayPixelFormat.type = GL_RENDERBUFFER;

    u16 displayWidth = 0;
    u16 displayHeight = 0;
    const u32 displayBuffer = (u32)gfxGetFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, &displayWidth, &displayHeight);

    GLASS_gx_transfer((u32)cb->address, cb->height, cb->width, GLASS_pixels_tryUnwrapTransferFormat(&colorPixelFormat),
        displayBuffer, displayWidth, displayHeight, GLASS_pixels_tryUnwrapTransferFormat(&displayPixelFormat),
        ctx->settings.verticalFlip, false, ctx->settings.transferScale, false, GLASS_displayTransferDone);
}