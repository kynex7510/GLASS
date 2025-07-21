/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Wrappers/TextureCopy.h>
#include <KYGX/Utility.h>
#include <RIP/Convert.h>
#include <RIP/Pixels.h>

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

void GLASS_read_colorBuffer(CtxCommon* ctx, u16 unalignedX, u16 unalignedY, u16 unalignedW, u16 unalignedH, RIPPixelFormat pixelFormat, u8* out) {
    // Get color buffer.
    const size_t fbIndex = GLASS_context_getFBIndex(ctx);
    const FramebufferInfo* fbInfo = (const FramebufferInfo*)ctx->framebuffer[fbIndex];
    KYGX_ASSERT(fbInfo);

    u8* cbAddr = NULL;
    u16 cbWidth = 0;
    u16 cbHeight = 0;
    GLenum cbFormat = 0;

    if (GLASS_OBJ_IS_RENDERBUFFER(fbInfo->colorBuffer)) {
        RenderbufferInfo* cb = (RenderbufferInfo*)fbInfo->colorBuffer;
        cbAddr = cb->address;
        cbWidth = cb->width;
        cbHeight = cb->height;
        cbFormat = cb->format;
    } else if (GLASS_OBJ_IS_TEXTURE(fbInfo->colorBuffer)) {
        RenderbufferInfo cb;
        TextureInfo* tex = (TextureInfo*)fbInfo->colorBuffer;
        GLASS_tex_getAsRenderbuffer(tex, fbInfo->texFace, &cb);
        cbAddr = cb.address;
        cbWidth = cb.width;
        cbHeight = cb.height;
        cbFormat = cb.format;
    } else {
        KYGX_UNREACHABLE("Invalid color buffer!");
    }

    // Setup surfaces and rects.
    const u16 alignedX = kygxAlignDown(unalignedX, 8);

    KYGXTextureCopySurface srcSurface;
    srcSurface.addr = cbAddr;
    srcSurface.width = cbWidth;
    srcSurface.height = cbHeight;
    srcSurface.pixelSize = tcPixelSize(cbFormat);
    srcSurface.rotated = true;

    void* rawData = glassLinearAlloc(width * height * srcSurface.pixelSize);
    KYGX_ASSERT(rawData);

    KYGXTextureCopySurface dstSurface;
    dstSurface.addr = rawData;
    dstSurface.width = width;
    dstSurface.height = height;
    dstSurface.pixelSize = srcSurface.pixelSize;
    dstSurface.rotated = true;

    KYGXTextureCopyRect srcRect;
    srcRect.x = x;
    srcRect.y = y;
    srcRect.width = width;
    srcRect.height = height;

    KYGXTextureCopyRect dstRect;
    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.width = width;
    dstRect.height = height;

    // Copy raw rect.
    kygxSyncRectCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);

    // Convert it from native.
    ripConvertInPlaceFromNative(rawData, width, height, getRIPPixelFormat(cbFormat), true);

    // Do pixel conversion.
    ripConvertPixels(rawData, out, width, height, getRIPPixelFormat(cbFormat), pixelFormat);

    // TODO: rotate.

    glassLinearFree(rawData);
}