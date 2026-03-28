/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Wrappers/TextureCopy.h>
#include <KYGX/Utility.h>
#include <RIP/Convert.h>
#include <RIP/Pixels.h>

#include "Base/Math.h"
#include "Base/Read.h"
#include "Base/TexManager.h"

static inline u8 tcPixelSize(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA8;
        case GL_RGB5_A1:
            return KYGX_TEXTURECOPY_PIXEL_SIZE_RGB5A1;
        case GL_RGB565:
            return KYGX_TEXTURECOPY_PIXEL_SIZE_RGB565;
        case GL_RGBA4:
            return KYGX_TEXTURECOPY_PIXEL_SIZE_RGBA4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
            return 0;
    }
}

static inline RIPPixelFormat getRIPPixelFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return RIP_PIXELFORMAT_RGBA8;
        case GL_RGB5_A1:
            return RIP_PIXELFORMAT_RGB5A1;
        case GL_RGB565:
            return RIP_PIXELFORMAT_RGB565;
        case GL_RGBA4:
            return RIP_PIXELFORMAT_RGBA4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
            return 0;
    }
}

void GLASS_read_colorBuffer(const FramebufferInfo* fb, u16 x, u16 y, u16 width, u16 height, RIPPixelFormat pixelFormat, u8* out) {
    KYGX_ASSERT(fb);
    KYGX_ASSERT(out);

    // Get color buffer.
    u8* cbAddr = NULL;
    u16 cbWidth = 0;
    u16 cbHeight = 0;
    GLenum cbFormat = 0;

    if (GLASS_OBJ_IS_RENDERBUFFER(fb->colorBuffer)) {
        RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
        cbAddr = cb->address;
        cbWidth = cb->width;
        cbHeight = cb->height;
        cbFormat = cb->format;
    } else if (GLASS_OBJ_IS_TEXTURE(fb->colorBuffer)) {
        RenderbufferInfo cb;
        TextureInfo* tex = (TextureInfo*)fb->colorBuffer;
        GLASS_tex_getAsRenderbuffer(tex, fb->texFace, &cb);
        cbAddr = cb.address;
        cbWidth = cb.width;
        cbHeight = cb.height;
        cbFormat = cb.format;
    } else {
        KYGX_UNREACHABLE("Invalid color buffer!");
    }

    // Setup surfaces and rects.
    const u16 alignedX = kygxAlignDown(x, 8);
    const u16 alignedY = kygxAlignDown(y, 8);
    const u16 offsetX = x - alignedX;
    const u16 offsetY = y - alignedY;
    const u16 alignedW = kygxAlignUp(width + offsetX, 8);
    const u16 alignedH = kygxAlignUp(height + offsetY, 8);

    KYGXTextureCopySurface srcSurface;
    srcSurface.addr = cbAddr;
    srcSurface.width = cbWidth;
    srcSurface.height = cbHeight;
    srcSurface.pixelSize = tcPixelSize(cbFormat);
    srcSurface.rotated = true;

    const size_t dstPixelSize = ripGetPixelFormatBPP(pixelFormat) >> 3;
    void* rawData = glassLinearAlloc(alignedW * alignedH * GLASS_MAX(dstPixelSize, srcSurface.pixelSize));
    KYGX_ASSERT(rawData);

    KYGXTextureCopySurface dstSurface;
    dstSurface.addr = rawData;
    dstSurface.width = alignedW;
    dstSurface.height = alignedH;
    dstSurface.pixelSize = srcSurface.pixelSize;
    dstSurface.rotated = true;

    KYGXTextureCopyRect srcRect;
    srcRect.x = alignedX;
    srcRect.y = alignedY;
    srcRect.width = dstSurface.width;
    srcRect.height = dstSurface.height;

    KYGXTextureCopyRect dstRect;
    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.width = dstSurface.width;
    dstRect.height = dstSurface.height;

    // Copy raw rect.
    kygxSyncRectCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);

    // Convert it from native.
    ripConvertInPlaceFromNative(dstSurface.addr, dstSurface.width, dstSurface.height, getRIPPixelFormat(cbFormat), true);

    // Do pixel conversion.
    ripConvertPixels(dstSurface.addr, dstSurface.addr, dstSurface.width, dstSurface.height, getRIPPixelFormat(cbFormat), pixelFormat);

    // Read lines.
    const size_t rowSize = width * dstPixelSize;
    const u8* src = (const u8*)dstSurface.addr;
    for (size_t i = 0; i < height; ++i) {
        const u8* row = &src[((i + offsetY) * rowSize) + (offsetX * dstPixelSize)];
        memcpy(&out[i * rowSize], row, rowSize);
    }

    glassLinearFree(dstSurface.addr);
}