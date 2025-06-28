#include <KYGX/Wrappers/DisplayTransfer.h>

#include "Base/Context.h"
#include "Platform/GPU.h"
#include "Platform/GFX.h"

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
    return glassCreateContext(&initParams, NULL);
}

void glassDestroyContext(GLASSCtx wrapped) {
    KYGX_ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)wrapped;

    if (ctx->initParams.version == GLASS_VERSION_2_0) {
        GLASS_context_cleanupCommon((CtxCommon*)ctx);
    } else {
        KYGX_UNREACHABLE("Invalid context version!");
    }

    glassHeapFree(ctx);
}

void glassBindContext(GLASSCtx ctx) { GLASS_context_bind((CtxCommon*)ctx); }
bool glassIsBoundContext(GLASSCtx ctx) { return GLASS_context_isBound((CtxCommon*)ctx); }

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

static void swapScreenBuffers(void* c) {
    CtxCommon* ctx = (CtxCommon*)c;
    KYGX_ASSERT(ctx);
    
    const bool stereo = ctx->settings.targetScreen == GLASS_SCREEN_TOP && ctx->settings.targetSide == GLASS_SIDE_RIGHT;
    GLASS_gfx_swapScreenBuffers(ctx->settings.targetScreen, stereo);
}

static inline u8 unwrapTransferFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return KYGX_DISPLAYTRANSFER_FMT_RGBA8;
        case GL_RGB5_A1:
            return KYGX_DISPLAYTRANSFER_FMT_RGB5A1;
        case GL_RGB565:
            return KYGX_DISPLAYTRANSFER_FMT_RGB565;
        case GL_RGBA4:
            return KYGX_DISPLAYTRANSFER_FMT_RGBA4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline u8 unwrapDownscale(GLASSDownscale downscale) {
    switch (downscale) {
        case GLASS_DOWNSCALE_NONE:
            return KYGX_DISPLAYTRANSFER_DOWNSCALE_NONE;
        case GLASS_DOWNSCALE_1X2:
            return KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1;
        case GLASS_DOWNSCALE_2X2:
            return KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }
    
    return 0;
}

static void transferScreenBuffers(CtxCommon* ctx) {
    KYGX_ASSERT(ctx);

    // Flush GPU commands.
    GLASS_context_flush(ctx, true);

    // Framebuffer might not be set.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer)) {
        swapScreenBuffers(ctx);
        return;
    }

    // Get color buffer.
    if (ctx->framebuffer == GLASS_INVALID_OBJECT) {
        swapScreenBuffers(ctx);
        return;
    }

    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
    if (fb->colorBuffer == GLASS_INVALID_OBJECT) {
        swapScreenBuffers(ctx);
        return;
    }

    const RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;

    // Get display buffer.
    u16 screenWidth;
    u16 screenHeight;
    u8* screenFb = GLASS_gfx_getFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, &screenWidth, &screenHeight);

    // Perform transfer.
    KYGXDisplayTransferFlags transferFlags;
    transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    transferFlags.srcFmt = unwrapTransferFormat(cb->format);
    transferFlags.dstFmt = unwrapTransferFormat(GLASS_gfx_framebufferFormat(ctx->settings.targetScreen));
    transferFlags.downscale = unwrapDownscale(ctx->settings.downscale);
    transferFlags.verticalFlip = ctx->settings.horizontalFlip;
    transferFlags.blockMode32 = false;

    if (transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1) {
        screenWidth <<= 1;
    } else if (transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2) {
        screenWidth <<= 1;
        screenHeight <<= 1;
    }

    const bool isBound = GLASS_context_isBound(ctx);
    if (isBound)
        kygxLock();

    kygxAddDisplayTransferChecked(&ctx->GXCmdBuf, cb->address, screenFb, cb->height, cb->width, screenWidth, screenHeight, &transferFlags);
    kygxCmdBufferFinalize(&ctx->GXCmdBuf, swapScreenBuffers, ctx);

    if (isBound)
        kygxUnlock(true);
}

void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom) {
    CtxCommon* topCtx = (CtxCommon*)top;
    CtxCommon* bottomCtx = (CtxCommon*)bottom;

    if (!topCtx && !bottomCtx)
        return;

    if (topCtx)
        transferScreenBuffers(topCtx);
    
    if (bottomCtx)
        transferScreenBuffers(bottomCtx);

    const bool anyVSync = (topCtx && topCtx->settings.vsync) || (bottomCtx && bottomCtx->settings.vsync);
    if (anyVSync && (GLASS_context_isBound(topCtx) || GLASS_context_isBound(bottomCtx))) {
        kygxWaitCompletion();
        kygxWaitVBlank();
    }
}

void glassSwapBuffers(void) { glassSwapContextBuffers((GLASSCtx)GLASS_context_getBound(), NULL); }