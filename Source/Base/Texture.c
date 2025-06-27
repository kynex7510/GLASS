#include "Base/Texture.h"
#include "Platform/GX.h"
#include "Base/Utility.h"
#include "Base/Pixels.h"

#include <string.h> // memset

size_t GLASS_tex_getNumFaces(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return 1;
        case GL_TEXTURE_CUBE_MAP:
            return 6;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_tex_set(TextureInfo* tex, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool vram, u8** faces) {
    ASSERT(tex);
    ASSERT(faces);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(GLASS_utility_isPowerOf2(width));
    ASSERT(GLASS_utility_isPowerOf2(height));
    ASSERT(pixelFormat);

    const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = tex->faces[i];
        tex->vram ? glassVRAMFree(p) : glassLinearFree(p);
        tex->faces[i] = faces[i];
    }

    memcpy(&tex->pixelFormat, pixelFormat, sizeof(glassPixelFormat));

    tex->width = width;
    tex->height = height;
    tex->vram = vram;
}

static bool GLASS_reallocTexImpl(TextureInfo* tex, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool vram) {
    ASSERT(tex);
    ASSERT(pixelFormat);

    u8* faces[GLASS_NUM_TEX_FACES];
    memset(faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));

    if (width && height) {
        const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
        const size_t allocSize = GLASS_tex_getAllocSize(width, height, pixelFormat, -1);

        for (size_t i = 0; i < numFaces; ++i) {
            faces[i] = vram ? glassVRAMAlloc(allocSize, VRAM_ALLOC_ANY) : glassLinearAlloc(allocSize);
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

    GLASS_tex_set(tex, width, height, pixelFormat, vram, faces);
    return true;
}

TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool vram) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(GLASS_utility_isPowerOf2(width));
    ASSERT(GLASS_utility_isPowerOf2(height));
    ASSERT(pixelFormat);

    const bool sameFormat = tex->pixelFormat.format == pixelFormat->format && tex->pixelFormat.type == pixelFormat->type;
    if ((tex->width == width) && (tex->height == height) && sameFormat && (tex->vram == vram))
        return TexReallocStatus_Unchanged;

    return GLASS_reallocTexImpl(tex, width, height, pixelFormat, vram) ? TexReallocStatus_Updated : TexReallocStatus_Failed;
}

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(glassIsLinear(data));
    ASSERT(size);

    const size_t mipmapOffset = GLASS_tex_getOffset(tex->width, tex->height, &tex->pixelFormat, level);
    u8* dst = tex->faces[face] + mipmapOffset;
    GLASS_gx_copy((u32)data, (u32)dst, size, size, 1, true);
}

void GLASS_tex_writeRaw(TextureInfo* tex, const u8* data, size_t face, size_t level) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);

    const size_t width = tex->width >> level;
    const size_t height = tex->height >> level;
    
    const size_t allocSize = (width * height * GLASS_pixels_bpp(&tex->pixelFormat)) >> 3;
    u8* flipped = glassLinearAlloc(allocSize);
    ASSERT(flipped);
    GLASS_pixels_flip(data, flipped, width, height, &tex->pixelFormat);

    u8* tiled = glassLinearAlloc(allocSize);
    ASSERT(tiled);
    GLASS_pixels_tiling(flipped, tiled, width, height, &tex->pixelFormat, true);
    glassLinearFree(flipped);

    GLASS_tex_write(tex, tiled, allocSize, face, level);
    glassLinearFree(tiled);
}