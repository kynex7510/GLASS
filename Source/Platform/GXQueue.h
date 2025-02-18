#ifndef _GLASS_PLATFORM_GX_H
#define _GLASS_PLATFORM_GX_H

#include "Base/Context.h"

#define GLASS_GX_CMD_PROCESS_COMMAND_LIST 0x01000101
#define GLASS_GX_CMD_MEMORY_FILL 0x01000102
#define GLASS_GX_CMD_DISPLAY_TRANSFER 0x01000103
#define GLASS_GX_CMD_TEXTURE_COPY 0x01000104

#define GLASS_GX_FILL_WIDTH_16 1
#define GLASS_GX_FILL_WIDTH_24 ((1 << 8) | 1)
#define GLASS_GX_FILL_WIDTH_32 ((2 << 8) | 1)

#define GLASS_GX_TRANSFER_FLAG_FMT_RGBA8 0
#define GLASS_GX_TRANSFER_FLAG_FMT_RGB8 1
#define GLASS_GX_TRANSFER_FLAG_FMT_RGB565 2
#define GLASS_GX_TRANSFER_FLAG_FMT_RGB5A1 3
#define GLASS_GX_TRANSFER_FLAG_FMT_RGBA4 4

#define GLASS_GX_TRANSFER_FLAG_VERTICAL_FLIP 0x1
#define GLASS_GX_TRANSFER_FLAG_MAKE_TILED 0x2
#define GLASS_GX_TRANSFER_FLAG_MIPMAP 0x20020

#define GLASS_GX_TRANSFER_DIMS(a, b) (((a) & 0xFFFF) << 16) | ((b) & 0xFFFF)
#define GLASS_GX_TRANSFER_SRCFMT(v) (((v) & 0x7) << 8)
#define GLASS_GX_TRANSFER_DSTFMT(v) (((v) & 0x7) << 12)

typedef struct {
    u32 data[8];
} GXCmd;

typedef void(*GXCallback_t)(void* param);

void GLASS_gx_runSync(GXCmd* cmd);
void GLASS_gx_runAsync(GXCmd* cmd, GXCallback_t callback, void* param, Barrier* barrier);

inline void GLASS_gx_makeProcessCommandList(GXCmd* cmd, const void* addr, size_t size, bool flush) {
    ASSERT(cmd);
    ASSERT(glassIsLinear(addr));

    cmd->data[0] = GLASS_GX_CMD_PROCESS_COMMAND_LIST;
    cmd->data[1] = (u32)addr;
    cmd->data[2] = size;
    cmd->data[3] = 0; // TODO: bit0 = gas related.
    cmd->data[4] = cmd->data[5] = cmd->data[6] = 0;
    cmd->data[7] = flush ? 0x1 : 0;
}

inline void GLASS_gx_makeMemoryFill(GXCmd* cmd, const void* addr0, size_t size0, u32 value0, u16 width0, const void* addr1, size_t size1, u32 value1, u16 width1) {
    ASSERT(cmd);
    ASSERT(addr0 || addr1);

    if (addr0) {
        ASSERT(glassIsLinear(addr0) || glassIsVRAM(addr0));
    }

    if (addr1) {
        ASSERT(glassIsLinear(addr1) || glassIsVRAM(addr1));
    }

    size_t offset = 0;
    if (!addr0 || ((u32)addr0 > (u32)addr1)) {
        const u32 tmp = width0;
        width0 = width1;
        width1 = tmp;
        offset = 3;
    }

    cmd->data[0] = GLASS_GX_CMD_MEMORY_FILL;
    cmd->data[1 + offset] = (u32)addr0;
    cmd->data[2 + offset] = value0;
    cmd->data[3 + offset] = (u32)addr0 + size0;
    cmd->data[4 - offset] = (u32)addr1;
    cmd->data[5 - offset] = value1;
    cmd->data[6 - offset] = (u32)addr1 + size1;
    cmd->data[7] = (width1 << 16) | width0;
}

inline void GLASS_gx_makeDisplayTransfer(GXCmd* cmd, const void* srcAddr, u16 srcWidth, u16 srcHeight, const void* dstAddr, u16 dstWidth, u16 dstHeight, u32 flags) {
    ASSERT(cmd);
    ASSERT(glassIsLinear(srcAddr) || glassIsVRAM(srcAddr));
    ASSERT(glassIsLinear(dstAddr) || glassIsVRAM(dstAddr));
    ASSERT(GLASS_utility_isAligned(srcWidth, 8));
    ASSERT(GLASS_utility_isAligned(srcHeight, 8));
    ASSERT(GLASS_utility_isAligned(dstWidth, 8));
    ASSERT(GLASS_utility_isAligned(dstHeight, 8));

    cmd->data[0] = GLASS_GX_CMD_DISPLAY_TRANSFER;
    cmd->data[1] = (u32)srcAddr;
    cmd->data[2] = (u32)dstAddr;
    cmd->data[3] = GLASS_GX_TRANSFER_DIMS(srcWidth, srcHeight);
    cmd->data[4] = GLASS_GX_TRANSFER_DIMS(dstWidth, dstHeight);
    cmd->data[5] = flags;
    cmd->data[6] = cmd->data[7] = 0;
}

inline void GLASS_gx_makeTextureCopy(GXCmd* cmd, const void* srcAddr, const void* dstAddr, size_t size, size_t stride, size_t count) {
    ASSERT(cmd);
    ASSERT(glassIsLinear(srcAddr) || glassIsVRAM(srcAddr));
    ASSERT(glassIsLinear(dstAddr) || glassIsVRAM(dstAddr));

    const u16 lineSize = size;
    const u16 gap = stride - size;
    size *= count;

    u32 flags = 0;
    if (gap) {
        ASSERT(size >= 192);
        ASSERT(lineSize);
        flags = 0x4;
    } else {
        ASSERT(size >= 16);
    }

    const u32 transferParams = GLASS_GX_TRANSFER_DIMS(lineSize >> 1, gap >> 1);

    cmd->data[0] = GLASS_GX_CMD_TEXTURE_COPY;
    cmd->data[1] = (u32)srcAddr;
    cmd->data[2] = (u32)dstAddr;
    cmd->data[3] = size;
    cmd->data[4] = transferParams;
    cmd->data[5] = transferParams;
    cmd->data[6] = 0x8 | flags;
    cmd->data[7] = 0;
}

#endif /* _GLASS_PLATFORM_GX_H */