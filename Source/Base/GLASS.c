#include "Base/Context.h"
#include "Base/Pixels.h"
#include "Platform/GX.h"
#include "Platform/GFX.h"
#include "Platform/Utility.h"

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

static void GLASS_swapScreenBuffers(CtxCommon* ctx) {
    ASSERT(ctx);
    const glassScreen screen = ctx->settings.targetScreen;
    const bool stereo = screen == GLASS_SCREEN_TOP && ctx->settings.targetSide == GLASS_SIDE_RIGHT;
    GLASS_gfx_swapScreenBuffers(screen, stereo);
}

static void GLASS_asyncSwapScreenBuffers(void* param) {
    CtxCommon* ctx = (CtxCommon*)param;
    GLASS_swapScreenBuffers(ctx);
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

void glassSwapContextBuffers(glassCtx top, glassCtx bottom) {
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

void glassSwapBuffers(void) { glassSwapContextBuffers((glassCtx)GLASS_context_getCommon(), NULL); }