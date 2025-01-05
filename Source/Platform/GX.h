#ifndef _GLASS_PLATFORM_GX_H
#define _GLASS_PLATFORM_GX_H

#include "Base/Context.h"
#include "Base/Pixels.h"

#define GLASS_GX_SET_WIDTH_16 0
#define GLASS_GX_SET_WIDTH_24 1
#define GLASS_GX_SET_WIDTH_32 2

typedef void (*GXTransferCallback_t)(gxCmdQueue_s*);

void GLASS_gx_init(CtxCommon* ctx);
void GLASS_gx_cleanup(CtxCommon* ctx);
void GLASS_gx_bind(CtxCommon* ctx);
void GLASS_gx_unbind(CtxCommon* ctx);

void GLASS_gx_copy(u32 srcAddr, u32 dstAddr, size_t size, size_t stride, size_t count, bool sync);
void GLASS_gx_set(u32 addr0, size_t size0, u32 value0, size_t width0, u32 addr1, size_t size1, u32 value1, size_t width1, bool sync);

void GLASS_gx_transfer(u32 srcAddr, size_t srcWidth, size_t srcHeight, GX_TRANSFER_FORMAT srcFmt,
    u32 dstAddr, size_t dstWidth, size_t dstHeight, GX_TRANSFER_FORMAT dstFmt,
    bool verticalFlip, bool makeTiled, GX_TRANSFER_SCALE scaling, bool sync, GXTransferCallback_t callback);

void GLASS_gx_sendGPUCommands(void);

#endif /* _GLASS_PLATFORM_GX_H */