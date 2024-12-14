#include "Base/Utility.h"
#include "Base/Context.h"
#include "Base/GPU.h"
#include "Base/GX.h"

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

    UNREACHABLE("Invalid GSP framebuffer format!");
}

void glassSwapBuffers(void) {
    CtxCommon* ctx = GLASS_context_getCommon();

    // Flush GPU commands.
    GLASS_context_flush();
    GLASS_gx_sendGPUCommands();

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
    displayBuffer.format = GLASS_wrapFBFormat(gfxGetScreenFormat(ctx->settings.targetScreen));
    displayBuffer.width = width;
    displayBuffer.height = height;

    GLASS_gx_transferAndSwap(colorBuffer, &displayBuffer);
}