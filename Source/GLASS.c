#include "Utility.h"
#include "Context.h"
#include "Memory.h"

static INLINE void GLASS_getDisplayBuffer(CtxCommon* ctx, RenderbufferInfo* displayBuffer) {
    u16 width = 0, height = 0;
    displayBuffer->address = gfxGetFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, &height, &width);
    displayBuffer->format = GLASS_utility_wrapFBFormat(gfxGetScreenFormat(ctx->settings.targetScreen));
    displayBuffer->width = width;
    displayBuffer->height = height;
}

static void GLASS_swapBuffersCb(gxCmdQueue_s* queue) {
    CtxCommon* ctx = (CtxCommon*)queue->user;
    gfxScreenSwapBuffers(ctx->settings.targetScreen, ctx->settings.targetScreen == GFX_TOP && ctx->settings.targetSide == GFX_RIGHT);
    gxCmdQueueSetCallback(queue, NULL, NULL);
}

glassCtx glassCreateContext(glassVersion version) { return glassCreateContextWithSettings(version, NULL); }

glassCtx glassCreateContextWithSettings(glassVersion version, const glassCtxSettings* settings) {
    if (version == GLASS_VERSION_1_1) {
        CtxV1* ctx = GLASS_virtualAlloc(sizeof(CtxV1));
        if (ctx)
            GLASS_context_initV1(ctx, settings);
        return (glassCtx)ctx;
    }

    if (version == GLASS_VERSION_2_0) {
        CtxV2* ctx = GLASS_virtualAlloc(sizeof(CtxV2));
        if (ctx)
            GLASS_context_initV2(ctx, settings);
        return (glassCtx)ctx;
    }

    return NULL;
}

void glassDestroyContext(glassCtx wrapped) {
    ASSERT(wrapped);
    CtxCommon* ctx = (CtxCommon*)ctx;
    
    if (ctx->version == GLASS_VERSION_1_1) {
        GLASS_context_cleanupV1((CtxV1*)ctx);
    } else if (ctx->version == GLASS_VERSION_2_0) {
        GLASS_context_cleanupV2((CtxV2*)ctx);
    }

    GLASS_virtualFree(ctx);
}

void glassReadSettings(glassCtx wrapped, glassCtxSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

    const CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(settings, &ctx->settings, sizeof(glassCtxSettings));
}

void glassWriteSettings(glassCtx wrapped, const glassCtxSettings* settings) {
    ASSERT(wrapped);
    ASSERT(settings);

    CtxCommon* ctx = (CtxCommon*)wrapped;
    memcpy(&ctx->settings, settings, sizeof(glassCtxSettings));
}

void glassBindContext(glassCtx* ctx) { GLASS_context_bind(ctx); }

void glassSwapBuffers(void) {
  RenderbufferInfo displayBuffer;
  memset(&displayBuffer, 0, sizeof(RenderbufferInfo));

  // Execute GPU commands.
  GLASS_context_update();
  CtxCommon* ctx = GLASS_context_getCommon();
  GLASS_gpu_flushAndRunCommands(ctx);

  // Framebuffer might not be set.
  if (!ObjectIsFramebuffer(ctx->framebuffer))
    return;

  // Get color buffer.
  FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
  RenderbufferInfo* colorBuffer = fb->colorBuffer;
  if (!colorBuffer)
    return;

  // Get display buffer.
  GLASS_getDisplayBuffer(ctx, &displayBuffer);
  ASSERT(displayBuffer.address);

  // Get transfer flags.
  const u32 transferFlags = MakeTransferFlags(false, false, false, GLASS_utility_getTransferFormat(fb->colorBuffer->format), GLASS_utility_getTransferFormat(displayBuffer.format), ctx->settings.transferScale);

  // Transfer buffer.
  GLASS_gpu_flushQueue(ctx, false);
  gxCmdQueueSetCallback(&ctx->gxQueue, GLASS_swapBuffersCb, (void*)ctx);
  GLASS_gpu_transferBuffer(fb->colorBuffer, &displayBuffer, transferFlags);
  GLASS_gpu_runQueue(ctx, false);
}