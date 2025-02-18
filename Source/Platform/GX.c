#include "Platform/GX.h"
#include "Platform/GPU.h"
#include "Base/Utility.h"

#define GX_CMD_PROCESS_COMMAND_LIST 0x01000101
#define GX_CMD_MEMORY_FILL 0x01000102
#define GX_CMD_DISPLAY_TRANSFER 0x01000103
#define GX_CMD_TEXTURE_COPY 0x01000104



typedef struct {
    u32 addr;
    size_t sizeInWords;
    bool flush;
} GXProcessCommandListParams;

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

static void GLASS_makeProcessCommandList(const GXProcessCommandListParams* cmdList, u32* out) {
    ASSERT(cmdList);
    ASSERT(glassIsLinear((void*)cmdList->addr));

    out[0] = GX_CMD_PROCESS_COMMAND_LIST;
    out[1] = cmdList->addr;
    out[2] = cmdList->sizeInWords * sizeof(u32);
    out[3] = 0; // TODO: bit0 = gas related.
    out[4] = out[5] = out[6] = 0;
    out[7] = cmdList->flush ? 0x1 : 0;
}

void GLASS_makeMemoryFill(const GXMemoryFillParams* fill0, const GXMemoryFillParams* fill1, u32* out) {
    ASSERT(fill0 || fill1);

    if (fill0) {
        ASSERT(glassIsLinear((void*)fill0->addr) || glassIsVRAM((void*)fill0->addr));
    }

    if (fill1) {
        ASSERT(glassIsLinear((void*)fill1->addr) || glassIsVRAM((void*)fill1->addr));
    }

    out[0] = GX_CMD_MEMORY_FILL;

    u32 fill1ctl = 0;
    if (fill0 && fill1)  {
        if (fill1->addr < fill0->addr) {
            const GXMemoryFillParams* tmp = fill0;
            fill0 = fill1;
            fill1 = tmp;
        }

        fill1ctl = FILL_CONTROL(fill1->width);

        out[4] = fill1->addr;
        out[5] = fill1->value;
        out[6] = fill1->addr + fill1->size;
    } else {
        out[4] = out[5] = out[6] = 0;
    }

    if (!fill0)
        fill0 = fill1;

    out[1] = fill0->addr;
    out[2] = fill0->value;
    out[3] = fill0->addr + fill0->size;
    
    out[7] = (fill1ctl << 16) | FILL_CONTROL(fill0->width);
}

static void GLASS_displayTransfer(const GXDisplayTransferParams* transfer, u32* out) {
    ASSERT(transfer);
    ASSERT(glassIsLinear((void*)transfer->srcAddr) || glassIsVRAM((void*)transfer->srcAddr));
    ASSERT(glassIsLinear((void*)transfer->dstAddr) || glassIsVRAM((void*)transfer->dstAddr));
    ASSERT(GLASS_utility_isAligned(transfer->srcWidth, 8));
    ASSERT(GLASS_utility_isAligned(transfer->srcHeight, 8));
    ASSERT(GLASS_utility_isAligned(transfer->dstWidth, 8));
    ASSERT(GLASS_utility_isAligned(transfer->dstHeight, 8));

    u32 flags = TRANSFER_FLAG_SRCFMT(transfer->srcFormat);
    flags |= TRANSFER_FLAG_DSTFMT(transfer->dstFormat);
    flags |= TRANSFER_FLAG_VFLIP(transfer->verticalFlip);
    flags |= TRANSFER_FLAG_MAKE_TILED(transfer->makeTiled);
    flags |= TRANSFER_FLAG_SCALING(transfer->scaling);

    out[0] = GX_CMD_DISPLAY_TRANSFER;
    out[1] = transfer->srcAddr;
    out[2] = transfer->dstAddr;
    out[3] = TRANSFER_DIMS(transfer->srcWidth, transfer->srcHeight);
    out[4] = TRANSFER_DIMS(transfer->dstWidth, transfer->dstHeight);
    out[5] = flags;
    out[6] = out[7] = 0;
}

static void GLASS_makeTextureCopy(const GXTextureCopyParams* copy, u32* out) {
    ASSERT(copy);
    ASSERT(glassIsLinear((void*)copy->srcAddr) || glassIsVRAM((void*)copy->srcAddr));
    ASSERT(glassIsLinear((void*)copy->dstAddr) || glassIsVRAM((void*)copy->dstAddr));

    u32 flags = 0;
    if (copy->gap) {
        ASSERT(copy->size >= 192);
        ASSERT(copy->lineSize);
        flags = 0x4;
    } else {
        ASSERT(copy->size >= 16);
    }

    const u32 transferParams = TRANSFER_DIMS(copy->lineSize >> 1, copy->gap >> 1);

    out[0] = GX_CMD_TEXTURE_COPY;
    out[1] = copy->srcAddr;
    out[2] = copy->dstAddr;
    out[3] = copy->size;
    out[4] = transferParams;
    out[5] = transferParams;
    out[6] = 0x8 | flags;
    out[7] = 0;
}

static void GLASS_submitGXCommandSync(const u32* cmdData) {
    if (R_SUCCEEDED(gspSubmitGxCommand(cmdData))) {
        switch (cmdData[])
    }
}

void gxCmdQueueInterrupt(GSPGPU_Event irq) {
    // Ignore vblank.
    if (irq == GSPGPU_EVENT_VBlank0 || irq == GSPGPU_EVENT_VBlank1)
        return;

    
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

void GLASS_gx_copy(u32 srcAddr, u32 dstAddr, size_t size, size_t stride, size_t count) {
    ASSERT(size);
    ASSERT(stride >= size);

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t flushSize = stride * count;
    ASSERT(GLASS_utility_flushCache((void*)srcAddr, flushSize));

    GXTextureCopyParams params;
    params.srcAddr = srcAddr;
    params.dstAddr = dstAddr;
    params.size = (size * count);
    params.lineSize = size;
    params.gap = (stride - size);

    u8 cmdData[8];
    GLASS_makeTextureCopy(&params, cmdData);

    GLASS_submitGXCommand();

    GLASS_textureCopy(&params);
    gspWaitForPPF();
    ASSERT(GLASS_utility_invalidateCache((void*)dstAddr, flushSize));
}

void GLASS_gx_set(u32 addr0, size_t size0, u32 value0, size_t width0, u32 addr1, size_t size1, u32 value1, size_t width1, bool sync) {
    GXMemoryFillParams fill0;
    GXMemoryFillParams fill1;

    if (addr0) {
        fill0.addr = addr0;
        fill0.size = size0;
        fill0.value = value0;
        fill0.width = width0;
    }

    if (addr1) {
        fill1.addr = addr1;
        fill1.size = size1;
        fill1.value = value1;
        fill1.width = width1;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    if (sync)
        GX_BindQueue(NULL);

    GLASS_memoryFill(addr0 ? &fill0 : NULL, addr1 ? &fill1 : NULL);

    if (sync) {
        gspWaitForPSC0();
        GX_BindQueue(&ctx->gxQueue);

        if (fill0.addr) {
            ASSERT(GLASS_utility_invalidateCache((void*)fill0.addr, fill0.size));
        }

        if (fill1.addr) {
            ASSERT(GLASS_utility_invalidateCache((void*)fill1.addr, fill1.size));
        }
    }
}

static size_t GLASS_transferFormatBpp(GX_TRANSFER_FORMAT format) {
    switch (format) {
        case GX_TRANSFER_FMT_RGBA8:
            return 32;
        case GX_TRANSFER_FMT_RGB8:
            return 24;
        case GX_TRANSFER_FMT_RGB565:
        case GX_TRANSFER_FMT_RGB5A1:
        case GX_TRANSFER_FMT_RGBA4:
            return 16;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gx_transfer(u32 srcAddr, size_t srcWidth, size_t srcHeight, GX_TRANSFER_FORMAT srcFmt,
    u32 dstAddr, size_t dstWidth, size_t dstHeight, GX_TRANSFER_FORMAT dstFmt,
    bool verticalFlip, bool makeTiled, GX_TRANSFER_SCALE scaling, bool sync, GXTransferCallback_t callback) {
    ASSERT(GLASS_utility_isAligned(srcWidth, 8));
    ASSERT(GLASS_utility_isAligned(srcHeight, 8));
    ASSERT(srcFmt != GLASS_INVALID_TRANSFER_FORMAT);
    ASSERT(GLASS_utility_isAligned(dstWidth, 8));
    ASSERT(GLASS_utility_isAligned(dstHeight, 8));
    ASSERT(dstFmt != GLASS_INVALID_TRANSFER_FORMAT);

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t srcFlushSize = ((srcWidth * srcHeight * GLASS_transferFormatBpp(srcFmt)) >> 3);
    ASSERT(GLASS_utility_flushCache((void*)srcAddr, srcFlushSize));

    GXDisplayTransferParams params;

    params.srcAddr = srcAddr;
    params.srcWidth = srcWidth;
    params.srcHeight = srcHeight;
    params.srcFormat = srcFmt;

    params.dstAddr = dstAddr;
    params.dstWidth = dstWidth;
    params.dstHeight = dstHeight;
    params.dstFormat = dstFmt;

    params.verticalFlip = verticalFlip;
    params.makeTiled = makeTiled;
    params.scaling = scaling;

    if (sync) {
        GX_BindQueue(NULL);
    } else if (callback) {
        gxCmdQueueWait(&ctx->gxQueue, -1);
        gxCmdQueueClear(&ctx->gxQueue);
        gxCmdQueueSetCallback(&ctx->gxQueue, callback, ctx);
    }

    GLASS_displayTransfer(&params);

    if (sync) {
        gspWaitForPPF();
        GX_BindQueue(&ctx->gxQueue);

        const size_t dstInvalidateSize = ((dstWidth * dstHeight * GLASS_transferFormatBpp(dstFmt)) >> 3);
        ASSERT(GLASS_utility_invalidateCache((void*)dstAddr, dstInvalidateSize));
    }
}

void GLASS_gx_sendGPUCommands(void) {
    CtxCommon* ctx = GLASS_context_getCommon();

    GXProcessCommandListParams params;
    void* addrTmp;
    if (!GLASS_gpu_swapCommandBuffers(&ctx->settings.gpuCmdList, &addrTmp, &params.sizeInWords))
        return;

    params.addr = (u32)addrTmp;
    params.sizeInWords >>= 2;

    if (ctx->initParams.flushAllLinearMem) {
        extern u32 __ctru_linear_heap;
        extern u32 __ctru_linear_heap_size;
        ASSERT(GLASS_utility_flushCache((void*)__ctru_linear_heap, __ctru_linear_heap_size));
        params.flush = false;
    } else {
        params.flush = true;
    }

    GLASS_processCommandList(&params);
}