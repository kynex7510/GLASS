#include <KYGX/Wrappers/DisplayTransfer.h>

#include "Base/Context.h"
#include "Platform/GPU.h"
#include "Platform/GFX.h"

#include <string.h> // memcpy

typedef struct {
    const void* src;
    void* dst;
    u16 srcWidth;
    u16 srcHeight;
    u16 dstWidth;
    u16 dstHeight;
    KYGXDisplayTransferFlags transferFlags;
} TransferParams;

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
GLASSCtx glassGetBoundContext(void) { return (GLASSCtx)GLASS_context_getBound(); }
bool glassHasBoundContext(void) { return GLASS_context_hasBound(); }
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

static inline u8 unwrapTransferFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return KYGX_DISPLAYTRANSFER_FMT_RGBA8;
        case GL_RGB8_OES:
            return KYGX_DISPLAYTRANSFER_FMT_RGB8;
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

static bool checkTransferFormats(u8 src, u8 dst) {
    // RGBA8 can convert into any other format.
    if (src == KYGX_DISPLAYTRANSFER_FMT_RGBA8)
        return true;

    // RGB8 can only convert into itself.
    if (src == KYGX_DISPLAYTRANSFER_FMT_RGB8)
        return src == dst;

    // other formats can only convert into other 16 bits formats.
    return dst == KYGX_DISPLAYTRANSFER_FMT_RGB565 ||
        dst == KYGX_DISPLAYTRANSFER_FMT_RGB5A1 ||
        dst == KYGX_DISPLAYTRANSFER_FMT_RGBA4;
}

static bool prepareTransferParams(CtxCommon* ctx, TransferParams* outParams, GLASSSide side) {
    if (!ctx)
        return false;

    size_t fbIndex = 0;
    if (ctx->settings.targetScreen == GLASS_SCREEN_TOP)
        fbIndex = (side == GLASS_SIDE_RIGHT) ? 1 : 0;

    // Flush GPU commands.
    GLASS_context_flush(ctx, true);

    // Framebuffer might not be set.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex]))
        return false;

    // Get color buffer.
    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer[fbIndex];
    if (fb->colorBuffer == GLASS_INVALID_OBJECT)
        return false;

    const RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
    outParams->src = cb->address;
    outParams->srcWidth = cb->height;
    outParams->srcHeight = cb->width;

    // Get display buffer.
    outParams->dst = GLASS_gfx_getFramebuffer(ctx->settings.targetScreen, side, &outParams->dstWidth, &outParams->dstHeight);

    // Setup transfer flags.
    outParams->transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    outParams->transferFlags.srcFmt = unwrapTransferFormat(cb->format);
    outParams->transferFlags.dstFmt = unwrapTransferFormat(GLASS_gfx_getFramebufferFormat(ctx->settings.targetScreen));
    outParams->transferFlags.downscale = unwrapDownscale(ctx->settings.downscale);
    outParams->transferFlags.verticalFlip = ctx->settings.horizontalFlip;
    outParams->transferFlags.blockMode32 = false;

    KYGX_BREAK_UNLESS(checkTransferFormats(outParams->transferFlags.srcFmt, outParams->transferFlags.dstFmt));

    if (outParams->transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1) {
        outParams->dstWidth <<= 1;
    } else if (outParams->transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2) {
        outParams->dstWidth <<= 1;
        outParams->dstHeight <<= 1;
    }

    return true;
}

static void swapBuffersCallback(void* screen) { GLASS_gfx_swapScreenBuffers((GLASSScreen)screen); }

void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom) {
    CtxCommon* topCtx = (CtxCommon*)top;
    CtxCommon* bottomCtx = (CtxCommon*)bottom;

    if (!topCtx && !bottomCtx)
        return;

    TransferParams topLeftParams;
    TransferParams topRightParams;
    const bool topLeftNeedsTransfer = prepareTransferParams(topCtx, &topLeftParams, GLASS_SIDE_LEFT);
    const bool topRightNeedsTransfer = prepareTransferParams(topCtx, &topRightParams, GLASS_SIDE_RIGHT);
    const bool topHasVSync = topCtx && topCtx->settings.vsync;

    TransferParams bottomParams;
    const bool bottomNeedsTransfer = prepareTransferParams(bottomCtx, &bottomParams, GLASS_SIDE_LEFT);
    const bool bottomHasVSync = bottomCtx && bottomCtx->settings.vsync;

    // Immediately swap buffers if no transfer is needed.
    if (!topLeftNeedsTransfer && !topRightNeedsTransfer)
        GLASS_gfx_swapScreenBuffers(GLASS_SCREEN_TOP);

    if (!bottomNeedsTransfer)
        GLASS_gfx_swapScreenBuffers(GLASS_SCREEN_BOTTOM);

    if (topLeftNeedsTransfer || topRightNeedsTransfer || bottomNeedsTransfer) {
        // Choose which command buffer to use.
        CtxCommon* transferCtx = NULL;
        bool useBoundCtx = false;

        if (GLASS_context_hasBound()) {
            transferCtx = GLASS_context_getBound();
            useBoundCtx = true;
        } else if (topCtx) {
            transferCtx = topCtx;
        } else {
            transferCtx = bottomCtx;
        }

        // If this is the currently bound context, we have to lock.
        if (useBoundCtx)
            kygxLock();

        // If bottom has vsync but top doesn't, execute bottom first.
        if (bottomHasVSync && !topHasVSync) {
            if (bottomNeedsTransfer) {
                kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, bottomParams.src, bottomParams.dst,
                    bottomParams.srcWidth, bottomParams.srcHeight, bottomParams.dstWidth, bottomParams.dstHeight,
                    &bottomParams.transferFlags);
                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)GLASS_SCREEN_BOTTOM);
            }

            if (topLeftNeedsTransfer || topRightNeedsTransfer) {
                if (topLeftNeedsTransfer) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, topLeftParams.src, topLeftParams.dst,
                        topLeftParams.srcWidth, topLeftParams.srcHeight, topLeftParams.dstWidth, topLeftParams.dstHeight,
                        &topLeftParams.transferFlags);
                }

                if (topRightNeedsTransfer) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, topRightParams.src, topRightParams.dst,
                        topRightParams.srcWidth, topRightParams.srcHeight, topRightParams.dstWidth, topRightParams.dstHeight,
                        &topRightParams.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)GLASS_SCREEN_TOP);
            }
        } else {
            // In any other case, we have to either wait for top, or none at all.
            if (topLeftNeedsTransfer || topRightNeedsTransfer) {
                if (topLeftNeedsTransfer) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, topLeftParams.src, topLeftParams.dst,
                        topLeftParams.srcWidth, topLeftParams.srcHeight, topLeftParams.dstWidth, topLeftParams.dstHeight,
                        &topLeftParams.transferFlags);
                }

                if (topRightNeedsTransfer) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, topRightParams.src, topRightParams.dst,
                        topRightParams.srcWidth, topRightParams.srcHeight, topRightParams.dstWidth, topRightParams.dstHeight,
                        &topRightParams.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)GLASS_SCREEN_TOP);
            }

            if (bottomNeedsTransfer) {
                kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, bottomParams.src, bottomParams.dst,
                    bottomParams.srcWidth, bottomParams.srcHeight, bottomParams.dstWidth, bottomParams.dstHeight,
                    &bottomParams.transferFlags);
                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)GLASS_SCREEN_BOTTOM);
            }
        }
        
        if (useBoundCtx) {
            kygxUnlock(true);
        } else {
            // If the transfer context is not bound, we have to bind it to execute commands.
            GLASS_context_bind(transferCtx);
            kygxFlushBufferedCommands();
        }

        // If any of the contexts has vsync enabled, wait for vblank.
        // TODO: only wait for the contexts that have vsync enabled (requires a primitive in kygx).
        if (topHasVSync || bottomHasVSync) {
            kygxWaitCompletion();
            kygxWaitVBlank();
        }
    }
}

void glassSwapBuffers(void) { glassSwapContextBuffers((GLASSCtx)GLASS_context_getBound(), NULL); }