#ifndef _GLASS_PLATFORM_GX_H
#define _GLASS_PLATFORM_GX_H

#include "Base/Context.h"

#define GLASS_GX_SET_WIDTH_16 1
#define GLASS_GX_SET_WIDTH_24 ((1 << 8) | 1)
#define GLASS_GX_SET_WIDTH_32 ((2 << 8) | 1)

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

void GLASS_gx_copy(const void* srcAddr, const void* dstAddr, size_t size, size_t stride, size_t count);
void GLASS_gx_set(u32 addr0, size_t size0, u32 value0, size_t width0, u32 addr1, size_t size1, u32 value1, size_t width1);

void GLASS_gx_transfer(const void* srcAddr, const void* dstAddr, u32 srcDims, u32 dstDims, u32 flags);

void GLASS_gx_sendGPUCommands(void);

#endif /* _GLASS_PLATFORM_GX_H */