#ifndef _GLASS_BASE_PIXELS_H
#define _GLASS_BASE_PIXELS_H

#include "Base/Types.h"

#define GLASS_INVALID_TEX_FORMAT ((GPU_TEXCOLOR)-1)
#define GLASS_INVALID_TRANSFER_FORMAT ((GX_TRANSFER_FORMAT)-1)

GPU_TEXCOLOR GLASS_pixels_tryUnwrapTexFormat(const glassPixelFormat* pixelFormat);
GX_TRANSFER_FORMAT GLASS_pixels_tryUnwrapTransferFormat(const glassPixelFormat* pixelFormat);
size_t GLASS_pixels_bpp(const glassPixelFormat* pixelFormat);

void GLASS_pixels_flip(const u8* src, u8* dst, size_t width, size_t height, const glassPixelFormat* pixelFormat);
void GLASS_pixels_tiling(const u8* src, u8* dst, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool makeTiled);

void GLASS_pixels_transfer(const u8* src, size_t srcWidth, size_t srcHeight, const glassPixelFormat* srcPixelFmt, bool srcTiled,
    u8* dst, size_t dstWidth, size_t dstHeight, const glassPixelFormat* dstPixelFmt, bool dstTiled);

#endif /* _GLASS_BASE_PIXELS_H */