/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Wrappers/DisplayTransfer.h>

#include "Base/Context.h"
#include "Base/TexManager.h"
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

GLASSCtx glassCreateContext(const GLASSCtxParams* ctxParams) {
    if (!ctxParams)
        return NULL;

    if (ctxParams->version == GLASS_VERSION_ES_2) {
        CtxCommon* ctx = (CtxCommon*)glassHeapAlloc(sizeof(CtxCommon));
        if (ctx) {
            GLASS_context_initCommon(ctx, ctxParams);
        }
        return (GLASSCtx)ctx;
    }

    return NULL;
}

void glassDestroyContext(GLASSCtx wrapped) {
    KYGX_ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)wrapped;

    if (ctx->params.version == GLASS_VERSION_ES_2) {
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

void glassFlushPendingGPUCommands(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    GLASS_context_flush((CtxCommon*)ctx, false);
}

GLASSVersion glassGetVersion(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.version;
}

GLASSScreen glassGetTargetScreen(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.targetScreen;
}

void glassSetTargetScreen(GLASSCtx ctx, GLASSScreen screen) {
    KYGX_ASSERT(ctx);
    ((CtxCommon*)ctx)->params.targetScreen = screen;
}

GLASSSide glassGetTargetSide(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.targetSide;
}

void glassSetTargetSide(GLASSCtx wrapped, GLASSSide side) {
    KYGX_ASSERT(wrapped);

    CtxCommon* ctx = (CtxCommon*)wrapped;

    // If we have changed side, we have to flush pending commands and request a framebuffer change.
    if (ctx->params.targetSide != side) {
        GLASS_context_flush(ctx, false);
        ctx->params.targetSide = side;
        ctx->flags |= GLASS_CONTEXT_FLAG_FRAMEBUFFER;
    }
}

void glassGetGPUCommandList(GLASSCtx ctx, GLASSGPUCommandList* list) {
    KYGX_ASSERT(ctx);
    KYGX_ASSERT(list);

    memcpy(list, &((CtxCommon*)ctx)->params.GPUCmdList, sizeof(GLASSGPUCommandList));
}

void glassSetGPUCommandList(GLASSCtx ctx, const GLASSGPUCommandList* list) {
    KYGX_ASSERT(ctx);
    KYGX_ASSERT(list);

    memcpy(&((CtxCommon*)ctx)->params.GPUCmdList, list, sizeof(GLASSGPUCommandList));
}

bool glassHasVSync(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.vsync;
}

void glassSetVSync(GLASSCtx ctx, bool enabled) {
    KYGX_ASSERT(ctx);
    ((CtxCommon*)ctx)->params.vsync = enabled;
}

bool glassHasHorizontalFlip(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.horizontalFlip;
}

void glassSetHorizontalFlip(GLASSCtx ctx, bool enabled) {
    KYGX_ASSERT(ctx);
    ((CtxCommon*)ctx)->params.horizontalFlip = enabled;
}

bool glassFlushesAllLinearMem(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.flushAllLinearMem;
}

void glassSetFlushAllLinearMem(GLASSCtx ctx, bool enabled) {
    KYGX_ASSERT(ctx);
    ((CtxCommon*)ctx)->params.flushAllLinearMem = enabled;
}

GLASSDownscale glassGetDownscale(GLASSCtx ctx) {
    KYGX_ASSERT(ctx);
    return ((CtxCommon*)ctx)->params.downscale;
}

void glassSetDownscale(GLASSCtx ctx, GLASSDownscale downscale) {
    KYGX_ASSERT(ctx);
    ((CtxCommon*)ctx)->params.downscale = downscale;
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

static void getTransferParams(CtxCommon* ctx, TransferParams* outParams, GLASSSide side) {
    KYGX_ASSERT(ctx);
    KYGX_ASSERT(outParams);

    // Bottom screen doesn't have a right framebuffer.
    if (ctx->params.targetScreen == GLASS_SCREEN_BOTTOM && side == GLASS_SIDE_RIGHT)
        return;

    // Get framebuffer index.
    const size_t fbIndex = (side == GLASS_SIDE_RIGHT) ? 1 : 0;

    // Framebuffer might not be set.
    if (!GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex]))
        return;

    // Get color buffer.
    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer[fbIndex];
    if (fb->colorBuffer == GLASS_INVALID_OBJECT)
        return;

    GLenum cbFormat;
    if (GLASS_OBJ_IS_RENDERBUFFER(fb->colorBuffer)) {
        const RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
        outParams->src = cb->address;
        outParams->srcWidth = cb->height;
        outParams->srcHeight = cb->width;
        cbFormat = cb->format;
    } else {
        KYGX_ASSERT(GLASS_OBJ_IS_TEXTURE(fb->colorBuffer));
        RenderbufferInfo cb;
        GLASS_tex_getAsRenderbuffer((const TextureInfo*)fb->colorBuffer, fb->texFace, &cb);
        outParams->src = cb.address;
        outParams->srcWidth = cb.height;
        outParams->srcHeight = cb.width;
        cbFormat = cb.format;
    }

    // Get display buffer.
    outParams->dst = GLASS_gfx_getFramebuffer(ctx->params.targetScreen, side, &outParams->dstWidth, &outParams->dstHeight);

    // Setup transfer flags.
    outParams->transferFlags.mode = KYGX_DISPLAYTRANSFER_MODE_T2L;
    outParams->transferFlags.srcFmt = unwrapTransferFormat(cbFormat);
    outParams->transferFlags.dstFmt = unwrapTransferFormat(GLASS_gfx_getFramebufferFormat(ctx->params.targetScreen));
    outParams->transferFlags.downscale = unwrapDownscale(ctx->params.downscale);
    outParams->transferFlags.verticalFlip = ctx->params.horizontalFlip;
    outParams->transferFlags.blockMode32 = false;

    KYGX_BREAK_UNLESS(checkTransferFormats(outParams->transferFlags.srcFmt, outParams->transferFlags.dstFmt));

    if (outParams->transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X1) {
        outParams->dstWidth <<= 1;
    } else if (outParams->transferFlags.downscale == KYGX_DISPLAYTRANSFER_DOWNSCALE_2X2) {
        outParams->dstWidth <<= 1;
        outParams->dstHeight <<= 1;
    }
}

static void prepareContextForTransfer(CtxCommon* ctx, TransferParams* leftParams, TransferParams* rightParams, bool* hasVSync) {
    KYGX_ASSERT(leftParams);
    KYGX_ASSERT(rightParams);
    KYGX_ASSERT(hasVSync);

    if (ctx) {
        // Bind this context to flush and execute pending commands.
        GLASS_context_bind(ctx);
        GLASS_context_flush(ctx, true);
        kygxWaitCompletion();

        // Get transfer params for each side.
        getTransferParams(ctx, leftParams, GLASS_SIDE_LEFT);
        getTransferParams(ctx, rightParams, GLASS_SIDE_RIGHT);

        // Get VSync.
        *hasVSync = ctx->params.vsync;
    }
}

static void swapBuffersCallback(void* screen) { GLASS_gfx_swapScreenBuffers((GLASSScreen)screen); }

void glassSwapContextBuffers(GLASSCtx wrapped0, GLASSCtx wrapped1) {
    if (!wrapped0 && !wrapped1)
        return;

    CtxCommon* ctx0 = (CtxCommon*)wrapped0;
    CtxCommon* ctx1 = (CtxCommon*)wrapped1;

    // Prepare the contexts for transfer.
    TransferParams left0Params;
    TransferParams right0Params;
    bool ctx0HasVSync = false;

    memset(&left0Params, 0, sizeof(TransferParams));
    memset(&right0Params, 0, sizeof(TransferParams));
    prepareContextForTransfer(ctx0, &left0Params, &right0Params, &ctx0HasVSync);

    TransferParams left1Params;
    TransferParams right1Params;
    bool ctx1HasVSync = false;

    memset(&left1Params, 0, sizeof(TransferParams));
    memset(&right1Params, 0, sizeof(TransferParams));
    prepareContextForTransfer(ctx1, &left1Params, &right1Params, &ctx1HasVSync);

    // Immediately swap buffers if no transfer is needed.
    bool ctx0NeedsSwap = true;
    if (!left0Params.src && !right0Params.src) {
        if (ctx0)
            GLASS_gfx_swapScreenBuffers(ctx0->params.targetScreen);

        ctx0NeedsSwap = false;
    }

    bool ctx1NeedsSwap = true;
    if (!left1Params.src && !right1Params.src) {
        if (ctx1)
            GLASS_gfx_swapScreenBuffers(ctx1->params.targetScreen);

        ctx1NeedsSwap = false;
    }

    // Handle swapping.
    if (ctx0NeedsSwap || ctx1NeedsSwap) {
        // Choose a command buffer to use.
        CtxCommon* transferCtx = NULL;

        if (GLASS_context_isBound(ctx0)) {
            transferCtx = ctx0;
        } else {
            KYGX_ASSERT(GLASS_context_isBound(ctx1));
            transferCtx = ctx1;
        }

        kygxLock();

        // If ctx1 has vsync but ctx0 doesn't, execute ctx1 first.
        if (ctx1HasVSync && !ctx0HasVSync) {
            if (ctx1NeedsSwap) {
                if (left1Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, left1Params.src, left1Params.dst,
                        left1Params.srcWidth, left1Params.srcHeight, left1Params.dstWidth, left1Params.dstHeight,
                        &left1Params.transferFlags);
                }

                if (right1Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, right1Params.src, right1Params.dst,
                        right1Params.srcWidth, right1Params.srcHeight, right1Params.dstWidth, right1Params.dstHeight,
                        &right1Params.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)ctx1->params.targetScreen);
            }

            if (ctx0NeedsSwap) {
                if (left0Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, left0Params.src, left0Params.dst,
                        left0Params.srcWidth, left0Params.srcHeight, left0Params.dstWidth, left0Params.dstHeight,
                        &left0Params.transferFlags);
                }

                if (right0Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, right0Params.src, right0Params.dst,
                        right0Params.srcWidth, right0Params.srcHeight, right0Params.dstWidth, right0Params.dstHeight,
                        &right0Params.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)ctx0->params.targetScreen);
            }
        } else {
            // In any other case, we have to either wait for ctx0, or none at all.
            if (ctx0NeedsSwap) {
                if (left0Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, left0Params.src, left0Params.dst,
                        left0Params.srcWidth, left0Params.srcHeight, left0Params.dstWidth, left0Params.dstHeight,
                        &left0Params.transferFlags);
                }

                if (right0Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, right0Params.src, right0Params.dst,
                        right0Params.srcWidth, right0Params.srcHeight, right0Params.dstWidth, right0Params.dstHeight,
                        &right0Params.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)ctx0->params.targetScreen);
            }

            if (ctx1NeedsSwap) {
                if (left1Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, left1Params.src, left1Params.dst,
                        left1Params.srcWidth, left1Params.srcHeight, left1Params.dstWidth, left1Params.dstHeight,
                        &left1Params.transferFlags);
                }

                if (right1Params.src) {
                    kygxAddDisplayTransferChecked(&transferCtx->GXCmdBuf, right1Params.src, right1Params.dst,
                        right1Params.srcWidth, right1Params.srcHeight, right1Params.dstWidth, right1Params.dstHeight,
                        &right1Params.transferFlags);
                }

                kygxCmdBufferFinalize(&transferCtx->GXCmdBuf, swapBuffersCallback, (void*)ctx1->params.targetScreen);
            }
        }
        
        kygxUnlock(true);

        // If any of the contexts has vsync enabled, wait for vblank.
        // TODO: only wait for the contexts that have vsync enabled (requires a primitive in kygx).
        if (ctx0HasVSync || ctx1HasVSync) {
            kygxWaitCompletion();
            kygxWaitVBlank();
        }
    }
}

void glassSwapBuffers(void) { glassSwapContextBuffers((GLASSCtx)GLASS_context_getBound(), NULL); }