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
        tex->faces[i] = faces[i];
    }

    tex->format = format;
    tex->width = width;
    tex->height = height;
    tex->vram = vram;
}

static inline bool GLASS_reallocTexImpl(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram) {
    KYGX_ASSERT(tex);

    u8* faces[GLASS_NUM_TEX_FACES];
    memset(faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));

    if (width && height) {
        const size_t numFaces = getNumFaces(tex->target);
        const size_t allocSize = ripGetTextureDataSize(width, height, getRIPPixelFormat(format), ripGetNumTextureLevels(width, height));

        for (size_t i = 0; i < numFaces; ++i) {
            faces[i] = vram ? glassVRAMAlloc(allocSize, KYGX_ALLOC_VRAM_BANK_ANY) : glassLinearAlloc(allocSize);
            if (!faces[i]) {
                // Free allocated buffers.
                for (size_t j = 0; j < i; ++j) {
                    u8* q = faces[j];
                    vram ? glassVRAMFree(q) : glassLinearFree(q);
                }

                GLASS_context_setError(GL_OUT_OF_MEMORY);
                return false;
            }
        }
    }

    GLASS_tex_setParams(tex, width, height, format, vram, faces);
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

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    KYGX_ASSERT(glassIsLinear(data));
    KYGX_ASSERT(size);

    const size_t mipmapOffset = ripGetTextureDataOffset(tex->width, tex->height, getRIPPixelFormat(tex->format), level);

    // Ensure the hardware can access data correctly.
    KYGXFlushCacheRegionsBuffer flushSrc;
    flushSrc.addr = data;
    flushSrc.size = size;

    KYGXFlushCacheRegionsBuffer flushDst;
    flushDst.addr = tex->faces[face] + mipmapOffset;
    flushDst.size = size;

    kygxSyncFlushCacheRegions(&flushSrc, &flushDst, NULL);
    kygxSyncTextureCopy(data, tex->faces[face] + mipmapOffset, size, 0, 0, 0, 0);

    // Avoid possible prefetches.
    kygxInvalidateDataCache(flushDst.addr, flushDst.size);
}

void GLASS_tex_writeUntiled(TextureInfo* tex, const u8* data, size_t face, size_t level) {
    KYGX_ASSERT(tex);
    KYGX_ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);

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
    GLASS_tex_write(tex, dst, size, face, level);
    glassLinearFree(dst);
}