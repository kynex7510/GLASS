/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Wrappers/TextureCopy.h>
#include <KYGX/Wrappers/FlushCacheRegions.h>
#include <KYGX/Utility.h>
#include <RIP/Texture.h>
#include <RIP/Convert.h>

#include "Base/Context.h"
#include "Base/TexManager.h"

#include <string.h> // memset

static inline size_t getNumFaces(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return 1;
        case GL_TEXTURE_CUBE_MAP:
            return 6;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline RIPPixelFormat getRIPPixelFormat(GPUTexFormat format) {
    switch (format) {
        case TEXFORMAT_RGBA8:
            return RIP_PIXELFORMAT_RGBA8;
        case TEXFORMAT_RGB8:
            return RIP_PIXELFORMAT_RGB8;
        case TEXFORMAT_RGB5A1:
            return RIP_PIXELFORMAT_RGB5A1;
        case TEXFORMAT_RGB565:
            return RIP_PIXELFORMAT_RGB565;
        case TEXFORMAT_RGBA4:
            return RIP_PIXELFORMAT_RGBA4;
        case TEXFORMAT_LA8:
            return RIP_PIXELFORMAT_LA8;
        case TEXFORMAT_HILO8:
            return RIP_PIXELFORMAT_HILO8;
        case TEXFORMAT_L8:
            return RIP_PIXELFORMAT_L8;
        case TEXFORMAT_A8:
            return RIP_PIXELFORMAT_A8;
        case TEXFORMAT_LA4:
            return RIP_PIXELFORMAT_LA4;
        case TEXFORMAT_L4:
            return RIP_PIXELFORMAT_L4;
        case TEXFORMAT_A4:
            return RIP_PIXELFORMAT_A4;
        case TEXFORMAT_ETC1:
            return RIP_PIXELFORMAT_ETC1;
        case TEXFORMAT_ETC1A4:
            return RIP_PIXELFORMAT_ETC1A4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_tex_getAsRenderbuffer(const TextureInfo* tex, size_t face, RenderbufferInfo* out) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(out);

    out->address = tex->faces[face];
    out->width = tex->width;
    out->height = tex->height;

    switch (tex->format) {
        case TEXFORMAT_RGBA8:
            out->format = GL_RGBA8_OES;
            break;
        case TEXFORMAT_RGB5A1:
            out->format = GL_RGB5_A1;
            break;
        case TEXFORMAT_RGB565:
            out->format = GL_RGB565;
            break;
        case TEXFORMAT_RGBA4:
            out->format = GL_RGBA4;
            break;
        default:
            KYGX_UNREACHABLE("Invalid format!");
            return;
    }

    out->bound = false;
}

void GLASS_tex_setParams(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram, u8** faces) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(faces);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(kygxIsPo2(width));
    KYGX_ASSERT(kygxIsPo2(height));

    const size_t numFaces = getNumFaces(tex->target);
    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = tex->faces[i];
        tex->vram ? glassVRAMFree(p) : glassLinearFree(p);
        tex->faces[i] = faces ? faces[i] : NULL;
    }

    tex->format = format;
    tex->width = width;
    tex->height = height;
    tex->vram = vram;
}

static inline bool GLASS_reallocTexImpl(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram) {
    KYGX_ASSERT(tex);

    GLASS_tex_setParams(tex, width, height, format, vram, NULL);

    if (width && height) {
        const size_t numFaces = getNumFaces(tex->target);
        const size_t allocSize = ripGetTextureDataSize(tex->width, tex->height, getRIPPixelFormat(tex->format), ripGetNumTextureLevels(tex->width, tex->height));

        for (size_t i = 0; i < numFaces; ++i) {
            tex->faces[i] = tex->vram ? glassVRAMAlloc(allocSize, KYGX_ALLOC_VRAM_BANK_ANY) : glassLinearAlloc(allocSize);
            const bool valid = tex->faces[i] && (i == 0 || ripValidateTextureFaceAddr(tex->faces[0], tex->faces[i]));

            if (!valid) {
                // Free allocated buffers.
                for (size_t j = 0; j < i; ++j) {
                    u8* q = tex->faces[j];
                    vram ? glassVRAMFree(q) : glassLinearFree(q);
                }

                GLASS_context_setError(GL_OUT_OF_MEMORY);
                return false;
            }
        }

        if (numFaces > 1)
            ripSortTextureFaces(tex->faces);
    }

    return true;
}

TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(kygxIsPo2(width));
    KYGX_ASSERT(kygxIsPo2(height));

    if (tex->format == format && tex->width == width && tex->height == height && tex->vram == vram)
        return TEXREALLOCSTATUS_UNCHANGED;

    return GLASS_reallocTexImpl(tex, width, height, format, vram) ? TEXREALLOCSTATUS_UPDATED : TEXREALLOCSTATUS_FAILED;
}

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t face, size_t level) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(glassIsLinear(data));
    KYGX_ASSERT(face < getNumFaces(tex->target));

    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t mipmapOffset = ripGetTextureDataOffset(tex->width, tex->height, pixelFormat, level);
    const size_t size = (tex->width * tex->height * ripGetPixelFormatBPP(pixelFormat)) >> 3;

    // Ensure the hardware can access data correctly.
    KYGXFlushCacheRegionsBuffer flushSrc;
    flushSrc.addr = data;
    flushSrc.size = size;

    u8* dst = tex->faces[face] + mipmapOffset;
    KYGXFlushCacheRegionsBuffer flushDst;
    flushDst.addr = dst;
    flushDst.size = size;

    kygxSyncFlushCacheRegions(&flushSrc, &flushDst, NULL);
    kygxSyncTextureCopy(data, dst, size, 0, 0, 0, 0);

    // Avoid possible prefetches.
    kygxInvalidateDataCache(flushDst.addr, flushDst.size);
}

void GLASS_tex_writeUntiled(TextureInfo* tex, const u8* data, size_t face, size_t level) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(data);

    const size_t width = tex->width >> level;
    const size_t height = tex->height >> level;
    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t size = (width * height * ripGetPixelFormatBPP(pixelFormat)) >> 3;

    const u8* src = data;

    // TODO: we could use the texture data if not in vram.
    u8* dst = glassLinearAlloc(size);
    KYGX_ASSERT(dst);

     // Allocate a temp buffer if data isn't in linearmem.
    const bool isOriginLinear = glassIsLinear(data);
   
    if (!isOriginLinear) {
        void* p = glassLinearAlloc(size);
        KYGX_ASSERT(p);
        memcpy(p, data, size);
        src = p;
    }

    // Do the conversion, with a flip on the Y axis (opengl coords are inverted).
    ripConvertToNative(src, dst, width, height, pixelFormat, true);

    if (!isOriginLinear)
        glassLinearFree((void*)src);

    // Write the converted data.
    GLASS_tex_write(tex, dst, face, level);
    glassLinearFree(dst);
}

void GLASS_tex_readRect(TextureInfo* tex, u8* dst, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(glassIsLinear(dst));    
    KYGX_ASSERT(face < getNumFaces(tex->target));
    KYGX_ASSERT(kygxIsAligned(x, 8));
    KYGX_ASSERT(kygxIsAligned(y, 8));
    KYGX_ASSERT(kygxIsAligned(width, 8));
    KYGX_ASSERT(kygxIsAligned(height, 8));

    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t bytesPerPixel = ripGetPixelFormatBPP(pixelFormat) >> 3;
    const size_t mipmapOffset = ripGetTextureDataOffset(tex->width, tex->height, pixelFormat, level);

    KYGXTextureCopySurface srcSurface;
    srcSurface.addr = tex->faces[face] + mipmapOffset;
    srcSurface.width = tex->width;
    srcSurface.height = tex->height;
    srcSurface.pixelSize = bytesPerPixel; // TODO
    srcSurface.rotated = true;

    KYGXTextureCopySurface dstSurface;
    dstSurface.addr = dst;
    dstSurface.width = width;
    dstSurface.height = height;
    dstSurface.pixelSize = bytesPerPixel; // TODO
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

    kygxSyncRectCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);
}

void GLASS_tex_writeRect(TextureInfo* tex, const u8* data, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(glassIsLinear(data));
    KYGX_ASSERT(face < getNumFaces(tex->target));
    KYGX_ASSERT(kygxIsAligned(x, 8));
    KYGX_ASSERT(kygxIsAligned(y, 8));
    KYGX_ASSERT(kygxIsAligned(width, 8));
    KYGX_ASSERT(kygxIsAligned(height, 8));
    
    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t bytesPerPixel = ripGetPixelFormatBPP(pixelFormat) >> 3;
    const size_t mipmapOffset = ripGetTextureDataOffset(tex->width, tex->height, pixelFormat, level);

    KYGXTextureCopySurface srcSurface;
    srcSurface.addr = (void*)data;
    srcSurface.width = width;
    srcSurface.height = height;
    srcSurface.pixelSize = bytesPerPixel; // TODO: use official macros.
    srcSurface.rotated = true;

    KYGXTextureCopySurface dstSurface;
    dstSurface.addr = tex->faces[face] + mipmapOffset;
    dstSurface.width = tex->width;
    dstSurface.height = tex->height;
    dstSurface.pixelSize = bytesPerPixel; // TODO
    dstSurface.rotated = true;

    KYGXTextureCopyRect srcRect;
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.width = width;
    srcRect.height = height;

    KYGXTextureCopyRect dstRect;
    dstRect.x = x;
    dstRect.y = y;
    dstRect.width = width;
    dstRect.height = height;

    kygxSyncRectCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);
}

void GLASS_tex_readUntiledRect(TextureInfo* tex, u8* dst, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(glassIsLinear(dst));
    KYGX_ASSERT(face < getNumFaces(tex->target));

    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t bytesPerPixel = ripGetPixelFormatBPP(pixelFormat) >> 3;

    const size_t alignedX = kygxAlignDown(x, 8);
    const size_t alignedY = kygxAlignDown(y, 8);
    const size_t alignedWidth = kygxAlignUp(width, 8);
    const size_t alignedHeight = kygxAlignUp(height, 8);

    u8* tmpRect = glassLinearAlloc(alignedWidth * alignedHeight * bytesPerPixel);
    KYGX_ASSERT(tmpRect);

    GLASS_tex_readRect(tex, tmpRect, face, level, alignedX, alignedY, alignedWidth, alignedHeight);
    ripConvertInPlaceFromNative(tmpRect, alignedWidth, alignedHeight, pixelFormat, true);

    const size_t lineWidth = width * bytesPerPixel;
    size_t srcOffset = ((y - alignedY) * alignedWidth * bytesPerPixel) + ((x - alignedX) * bytesPerPixel);
    size_t dstOffset = 0;

    for (size_t i = 0; i < height; ++i) {
        memcpy(dst + dstOffset, tmpRect + srcOffset, lineWidth);
        srcOffset += alignedWidth * bytesPerPixel;
        dstOffset += lineWidth;
    }

    glassLinearFree(tmpRect);
}

void GLASS_tex_writeUntiledRect(TextureInfo* tex, const u8* data, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(data);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);

    const RIPPixelFormat pixelFormat = getRIPPixelFormat(tex->format);
    const size_t bytesPerPixel = ripGetPixelFormatBPP(pixelFormat) >> 3;

    const size_t alignedX = kygxAlignDown(x, 8);
    const size_t alignedY = kygxAlignDown(y, 8);
    const size_t alignedWidth = kygxAlignUp(width, 8);
    const size_t alignedHeight = kygxAlignUp(height, 8);

    u8* tmpRect = glassLinearAlloc(alignedWidth * alignedHeight * bytesPerPixel);
    KYGX_ASSERT(tmpRect);

    GLASS_tex_readUntiledRect(tex, tmpRect, face, level, alignedX, alignedY, alignedWidth, alignedHeight);

    // Write data.
    const size_t lineWidth = width * bytesPerPixel;
    size_t srcOffset = 0;
    size_t dstOffset = ((y - alignedY) * alignedWidth * bytesPerPixel) + ((x - alignedX) * bytesPerPixel);

    for (size_t i = 0; i < height; ++i) {
        memcpy(tmpRect + dstOffset, data + srcOffset, lineWidth);
        srcOffset += lineWidth;
        dstOffset += alignedWidth * bytesPerPixel;
    }

    ripConvertInPlaceToNative(tmpRect, alignedWidth, alignedHeight, pixelFormat, true);
    GLASS_tex_writeRect(tex, tmpRect, face, level, alignedX, alignedY, alignedWidth, alignedHeight);
    glassLinearFree(tmpRect);
}