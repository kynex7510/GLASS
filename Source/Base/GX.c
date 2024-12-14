#include "Base/GX.h"
#include "Base/GPU.h"
#include "Base/Utility.h"

#define MAX_ENTRIES 32

#define FILL_CONTROL(fillWidth) (((fillWidth) << 8) | 1)

#define TRANSFER_FLAGS(srcFormat, dstFormat, verticalFlip, makeTiled, scaling) \
    (GX_TRANSFER_IN_FORMAT(srcFormat) | GX_TRANSFER_OUT_FORMAT(dstFormat) | GX_TRANSFER_FLIP_VERT(verticalFlip) | GX_TRANSFER_OUT_TILED(makeTiled) | GX_TRANSFER_SCALING(scaling))

typedef struct {
    u32 addr;
    size_t size;
    u32 value;
    u16 width;
} GXMemoryFillParams;

typedef struct {
    u32 srcAddr;
    u16 srcWidth;
    u16 srcHeight;
    GX_TRANSFER_FORMAT srcFormat;
    u32 dstAddr;
    u16 dstWidth;
    u16 dstHeight;
    GX_TRANSFER_FORMAT dstFormat;
    bool verticalFlip;
    bool makeTiled;
    GX_TRANSFER_SCALE scaling;
} GXDisplayTransferParams;

typedef struct {
    u32 srcAddr;
    u32 dstAddr;
    size_t size;
} GXTextureCopyParams;

typedef struct {
    u32 addr;
    size_t sizeInWords;
    bool flush;
} GXProcessCommandListParams;

void GLASS_memoryFill(const GXMemoryFillParams* fill0, const GXMemoryFillParams* fill1) {
    ASSERT(fill0 || fill1);

    if (fill0) {
        ASSERT(glassIsLinear((void*)fill0->addr) || glassIsVRAM((void*)fill0->addr));
    }

    if (fill1) {
        ASSERT(glassIsLinear((void*)fill1->addr) || glassIsVRAM((void*)fill1->addr));
    }

    if (fill0 && fill1) {
        if (fill1->addr < fill0->addr) {
            const GXMemoryFillParams* tmp = fill0;
            fill0 = fill1;
            fill1 = tmp;
        }

        GX_MemoryFill((u32*)fill0->addr, fill0->value, (u32*)(fill0->addr + fill0->size), FILL_CONTROL(fill0->width),
            (u32*)fill1->addr, fill1->value, (u32*)(fill1->addr + fill1->size), FILL_CONTROL(fill1->width));
    } else {
        if (!fill0)
            fill0 = fill1;

        GX_MemoryFill((u32*)fill0->addr, fill0->value, (u32*)(fill0->addr + fill0->size), FILL_CONTROL(fill0->width), NULL, 0, NULL, 0);
    }
}

static void GLASS_displayTransfer(const GXDisplayTransferParams* transfer) {
    ASSERT(transfer);
    ASSERT(glassIsLinear((void*)transfer->srcAddr) || glassIsVRAM((void*)transfer->srcAddr));
    ASSERT(glassIsLinear((void*)transfer->dstAddr) || glassIsVRAM((void*)transfer->dstAddr));

    GX_DisplayTransfer((u32*)(transfer->srcAddr), GX_BUFFER_DIM(transfer->srcHeight, transfer->srcWidth),
        (u32*)(transfer->dstAddr), GX_BUFFER_DIM(transfer->dstHeight, transfer->dstWidth),
        TRANSFER_FLAGS(transfer->srcFormat, transfer->dstFormat, transfer->verticalFlip, transfer->makeTiled, transfer->scaling));
}

static void GLASS_textureCopy(const GXTextureCopyParams* copy) {
    ASSERT(copy);
    ASSERT(glassIsLinear((void*)copy->srcAddr) || glassIsVRAM((void*)copy->srcAddr));
    ASSERT(glassIsLinear((void*)copy->dstAddr) || glassIsVRAM((void*)copy->dstAddr));
    GX_TextureCopy((u32*)copy->srcAddr, 0, (u32*)copy->dstAddr, 0, copy->size, 0x8 | GX_TRANSFER_OUT_TILED(true));
}

static void GLASS_processCommandList(const GXProcessCommandListParams* cmdList) {
    ASSERT(cmdList);
    ASSERT(glassIsLinear((void*)cmdList->addr));
    GX_ProcessCommandList((u32*)cmdList->addr, cmdList->sizeInWords * sizeof(u32), cmdList->flush ? GX_CMDLIST_FLUSH : 0);
}

void GLASS_gx_init(CtxCommon* ctx) {
    ASSERT(ctx);

    ctx->gxQueue.maxEntries = MAX_ENTRIES;
    ctx->gxQueue.entries = (gxCmdEntry_s*)glassVirtualAlloc(ctx->gxQueue.maxEntries * sizeof(gxCmdEntry_s));
    ASSERT(ctx->gxQueue.entries);
}

void GLASS_gx_cleanup(CtxCommon* ctx) {
    ASSERT(ctx);

    GLASS_gx_unbind(ctx);
    glassVirtualFree(ctx->gxQueue.entries);
}

void GLASS_gx_bind(CtxCommon* ctx) {
    ASSERT(ctx);

    GX_BindQueue(&ctx->gxQueue);
    gxCmdQueueRun(&ctx->gxQueue);
}

void GLASS_gx_unbind(CtxCommon* ctx) {
    ASSERT(ctx);

    gxCmdQueueWait(&ctx->gxQueue, -1);
    gxCmdQueueStop(&ctx->gxQueue);
    gxCmdQueueClear(&ctx->gxQueue);
    GX_BindQueue(NULL);
}

void GLASS_gx_clearBuffers(RenderbufferInfo* colorBuffer, u32 colorClear, RenderbufferInfo* depthBuffer, u32 depthClear) {
    GXMemoryFillParams colorFill;
    GXMemoryFillParams depthFill;

    if (!colorBuffer && !depthBuffer)
        return;

    // Flush GPU commands to enforce draw order.
    GLASS_context_flush();
    GLASS_gx_sendGPUCommands();

    if (colorBuffer) {
        colorFill.addr = (u32)colorBuffer->address;
        colorFill.size = (colorBuffer->width * colorBuffer->height * (GLASS_utility_getRenderbufferBPP(colorBuffer->format) >> 3));
        colorFill.value = colorClear;
        colorFill.width = GLASS_utility_unwrapRenderbufferPixelSize(colorBuffer->format);
    }

    if (depthBuffer) {
        depthFill.addr = (u32)depthBuffer->address;
        depthFill.size = (depthBuffer->width * depthBuffer->height * (GLASS_utility_getRenderbufferBPP(depthBuffer->format) >> 3));
        depthFill.value = depthClear;
        depthFill.width = GLASS_utility_unwrapRenderbufferPixelSize(depthBuffer->format);
    }

    GLASS_memoryFill(colorBuffer ? &colorFill : NULL, depthBuffer ? &depthFill : NULL);
}

static void GLASS_displayTransferDone(gxCmdQueue_s* queue) {
    CtxCommon* ctx = (CtxCommon*)queue->user;
    gfxScreenSwapBuffers(ctx->settings.targetScreen, ctx->settings.targetScreen == GFX_TOP && ctx->settings.targetSide == GFX_RIGHT);
    gxCmdQueueSetCallback(queue, NULL, NULL);
}

static GX_TRANSFER_FORMAT GLASS_unwrapTransferFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return GX_TRANSFER_FMT_RGBA8;
        case GL_RGB8_OES:
            return GX_TRANSFER_FMT_RGB8;
        case GL_RGB565:
            return GX_TRANSFER_FMT_RGB565;
        case GL_RGB5_A1:
            return GX_TRANSFER_FMT_RGB5A1;
        case GL_RGBA4:
            return GX_TRANSFER_FMT_RGBA4;
    }

    UNREACHABLE("Invalid transfer format!");
}

void GLASS_gx_transferAndSwap(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer) {
    ASSERT(colorBuffer);
    ASSERT(displayBuffer);

    CtxCommon* ctx = GLASS_context_getCommon();
    GXDisplayTransferParams params;

    params.srcAddr = (u32)colorBuffer->address;
    params.srcWidth = colorBuffer->width;
    params.srcHeight = colorBuffer->height;
    params.srcFormat = GLASS_unwrapTransferFormat(colorBuffer->format);

    params.dstAddr = (u32)displayBuffer->address;
    params.dstWidth = displayBuffer->width;
    params.dstHeight = displayBuffer->height;
    params.dstFormat = GLASS_unwrapTransferFormat(displayBuffer->format);

    params.verticalFlip = ctx->settings.verticalFlip;
    params.makeTiled = false;
    params.scaling = ctx->settings.transferScale;

    gxCmdQueueWait(&ctx->gxQueue, -1);
    gxCmdQueueClear(&ctx->gxQueue);
    gxCmdQueueSetCallback(&ctx->gxQueue, GLASS_displayTransferDone, ctx);
    GLASS_displayTransfer(&params);
}

void GLASS_gx_copyTexture(u32 srcAddr, u32 dstAddr, size_t size) {
    CtxCommon* ctx = GLASS_context_getCommon();

    GXTextureCopyParams params;
    params.srcAddr = srcAddr;
    params.dstAddr = dstAddr;
    params.size = size;

    GX_BindQueue(NULL);
    GLASS_textureCopy(&params);
    gspWaitForPPF();
    GX_BindQueue(&ctx->gxQueue);
}

void GLASS_gx_transformTexture(u32 srcAddr, u32 dstAddr, const TexTransformationParams* cvtParams) {
    ASSERT(cvtParams);
    CtxCommon* ctx = GLASS_context_getCommon();
    GXDisplayTransferParams params;
    params.srcAddr = srcAddr;
    params.srcWidth = cvtParams->inputWidth;
    params.srcHeight = cvtParams->inputHeight;
    params.srcFormat = GLASS_unwrapTransferFormat(cvtParams->inputFormat);
    params.dstAddr = dstAddr;
    params.dstWidth = cvtParams->outputWidth;
    params.dstHeight = cvtParams->outputHeight;
    params.dstFormat = GLASS_unwrapTransferFormat(cvtParams->outputFormat);
    params.verticalFlip = cvtParams->verticalFlip;
    params.makeTiled = cvtParams->makeTiled;
    params.scaling = GX_TRANSFER_SCALE_NO;
    GX_BindQueue(NULL);
    GLASS_displayTransfer(&params);
    gspWaitForPPF();
    GX_BindQueue(&ctx->gxQueue);
}

void GLASS_gx_sendGPUCommands(void) {
    CtxCommon* ctx = GLASS_context_getCommon();

    GXProcessCommandListParams params;
    if (!GLASS_gpu_swapCommandBuffers(&params.addr, &params.sizeInWords))
        return;

    if (ctx->initParams.flushAllLinearMem) {
        extern u32 __ctru_linear_heap;
        extern u32 __ctru_linear_heap_size;
        ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache((void*)__ctru_linear_heap, __ctru_linear_heap_size)));
        params.flush = false;
    } else {
        params.flush = true;
    }

    GLASS_processCommandList(&params);
}