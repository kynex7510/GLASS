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
    u16 lineSize;
    u16 gap;
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

    u32 flags = 0;
    if (copy->gap) {
        ASSERT(copy->size >= 192);
        ASSERT(copy->lineSize);
        flags |= 0x4;
    } else {
        ASSERT(copy->size >= 16);
    }

    GX_TextureCopy((u32*)copy->srcAddr, GX_BUFFER_DIM(copy->lineSize >> 1, copy->gap >> 1),
    (u32*)copy->dstAddr, GX_BUFFER_DIM(copy->lineSize >> 1, copy->gap >> 1), copy->size, 0x8 | flags);
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

static u16 GLASS_getFillWidth(GLenum format) { return GLASS_utility_getRenderbufferBpp(format) >> 4; }

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
        colorFill.size = (colorBuffer->width * colorBuffer->height * (GLASS_utility_getRenderbufferBpp(colorBuffer->format) >> 3));
        colorFill.value = colorClear;
        colorFill.width = GLASS_getFillWidth(colorBuffer->format);
    }

    if (depthBuffer) {
        depthFill.addr = (u32)depthBuffer->address;
        depthFill.size = (depthBuffer->width * depthBuffer->height * (GLASS_utility_getRenderbufferBpp(depthBuffer->format) >> 3));
        depthFill.value = depthClear;
        depthFill.width = GLASS_getFillWidth(depthBuffer->format);
    }

    GLASS_memoryFill(colorBuffer ? &colorFill : NULL, depthBuffer ? &depthFill : NULL);
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

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gx_transferAndSwap(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer, void (*callback)(gxCmdQueue_s*)) {
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

    if (callback) {
        gxCmdQueueWait(&ctx->gxQueue, -1);
        gxCmdQueueClear(&ctx->gxQueue);
        gxCmdQueueSetCallback(&ctx->gxQueue, callback, ctx);
    }
    
    GLASS_displayTransfer(&params);
}

void GLASS_gx_copyTexture(const GXTexCopy* copy) {
    ASSERT(copy);
    ASSERT(copy->size);
    ASSERT(copy->stride >= copy->size);

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t flushSize = copy->stride * copy->count;
    ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache((void*)copy->srcAddr, flushSize)));

    GXTextureCopyParams params;
    params.srcAddr = copy->srcAddr;
    params.dstAddr = copy->dstAddr;
    params.size = (copy->size * copy->count);
    params.lineSize = copy->size;
    params.gap = (copy->stride - copy->size);

    GX_BindQueue(NULL);
    GLASS_textureCopy(&params);
    gspWaitForPPF();
    GX_BindQueue(&ctx->gxQueue);

    ASSERT(R_SUCCEEDED(GSPGPU_InvalidateDataCache((void*)copy->dstAddr, flushSize)));
}

void GLASS_gx_applyTiling(const GXTexTiling* tiling) {
    ASSERT(tiling);

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t flushSize = tiling->width * tiling->height * GLASS_utility_getRenderbufferBpp(tiling->format);
    ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache((void*)tiling->srcAddr, flushSize)));

    GXDisplayTransferParams params;

    params.srcAddr = tiling->srcAddr;
    params.srcWidth = tiling->width;
    params.srcHeight = tiling->height;
    params.srcFormat = GLASS_unwrapTransferFormat(tiling->format);

    params.dstAddr = tiling->dstAddr;
    params.dstWidth = params.srcWidth;
    params.dstHeight = params.srcHeight;
    params.dstFormat = params.srcFormat;

    params.verticalFlip = false;
    params.makeTiled = tiling->makeTiled;
    params.scaling = GX_TRANSFER_SCALE_NO;
    
    GX_BindQueue(NULL);
    GLASS_displayTransfer(&params);
    gspWaitForPPF();
    GX_BindQueue(&ctx->gxQueue);

    ASSERT(R_SUCCEEDED(GSPGPU_InvalidateDataCache((void*)tiling->dstAddr, flushSize)));
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