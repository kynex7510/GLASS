#include "Utility.h"
#include "Context.h"
#include "Memory.h"
#include "GPU.h"

glassCtx glassCreateContext(glassVersion version) { return glassCreateContextWithSettings(version, NULL); }

glassCtx glassCreateContextWithSettings(glassVersion version, const glassSettings* settings) {
    if (version == GLASS_VERSION_1_1) {
        CtxV1* ctx = GLASS_virtualAlloc(sizeof(CtxV1));
        if (ctx)
            GLASS_context_initV1(ctx, settings);
        return (glassCtx)ctx;
    }

    return NULL;
}

void glassDestroyContext(glassCtx wrapped) {
    ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)wrapped;
    
    if (ctx->version == GLASS_VERSION_1_1) {
        GLASS_context_cleanupV1((CtxV1*)ctx);
    } else {
        UNREACHABLE("Invalid constext version!");
    }

    GLASS_virtualFree(ctx);
}

void glassReadSettings(glassCtx wrapped, glassSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

    const CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(settings, &ctx->settings, sizeof(glassSettings));
}

void glassWriteSettings(glassCtx wrapped, const glassSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

    CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(&ctx->settings, settings, sizeof(glassSettings));
}

void glassBindContext(glassCtx ctx) { GLASS_context_bind((CtxCommon*)ctx); }

static void GLASS_swapBuffersCb(gxCmdQueue_s* queue) {
    // TODO: we have a race here.
    CtxCommon* ctx = (CtxCommon*)queue->user;
    gfxScreenSwapBuffers(ctx->settings.targetScreen, ctx->settings.targetScreen == GFX_TOP && ctx->settings.targetSide == GFX_RIGHT);
    gxCmdQueueSetCallback(queue, NULL, NULL);
}

void glassSwapBuffers(void) {
    // Execute GPU commands.
    GLASS_context_update();
    CtxCommon* ctx = GLASS_context_getCommon();
    GLASS_gpu_flushAndRunCommands(ctx);

    // Framebuffer might not be set.
    if (!OBJ_IS_FRAMEBUFFER(ctx->framebuffer))
        return;

    // Get color buffer.
    FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
    RenderbufferInfo* colorBuffer = fb->colorBuffer;
    if (!colorBuffer)
        return;

    // Get display buffer.
    RenderbufferInfo displayBuffer;
    u16 width = 0, height = 0;
    displayBuffer.address = gfxGetFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, &height, &width);
    ASSERT(displayBuffer.address);
    displayBuffer.format = GLASS_utility_wrapFBFormat(gfxGetScreenFormat(ctx->settings.targetScreen));
    displayBuffer.width = width;
    displayBuffer.height = height;

    // Get transfer flags.
    const u32 transferFlags = GLASS_utility_makeTransferFlags(false, false, false, GLASS_utility_getTransferFormat(fb->colorBuffer->format), GLASS_utility_getTransferFormat(displayBuffer.format), ctx->settings.transferScale);

    // Transfer buffer.
    GLASS_gpu_flushQueue(ctx, false);
    gxCmdQueueSetCallback(&ctx->gxQueue, GLASS_swapBuffersCb, (void*)ctx);
    GLASS_gpu_transferBuffer(fb->colorBuffer, &displayBuffer, transferFlags);
    GLASS_gpu_runQueue(ctx, false);
}