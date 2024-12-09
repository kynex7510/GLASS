#include "TexCommon.h"
#include "Utility.h"
#include "GX.h"

#include <string.h>

static size_t GLASS_getTexBitsPerPixel(GLenum format, GLenum dataType) {
    switch (GLASS_utility_getTexFormat(format, dataType)) {
        case GPU_RGBA8:
            return 32;
        case GPU_RGB8:
            return 24;
        case GPU_RGBA5551:
        case GPU_RGB565:
        case GPU_RGBA4:
        case GPU_LA8:
        case GPU_HILO8:
            return 16;
        case GPU_L8:
        case GPU_A8:
        case GPU_LA4:
        case GPU_ETC1A4:
            return 8;
        case GPU_L4:
        case GPU_A4:
        case GPU_ETC1:
            return 4;
    }

    UNREACHABLE("Invalid GPU texture format!");
}

size_t GLASS_tex_getNumFaces(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return 1;
        case GL_TEXTURE_CUBE_MAP:
            return 6;
    }

    UNREACHABLE("Invalid texture target!");
}

size_t GLASS_tex_getOffset(u16 width, u16 height, GLenum format, GLenum type, size_t level) {
    ASSERT(level <= GLASS_NUM_TEX_LEVELS);

    if (level > 0) {
        // Basically rewrite the offset in terms of the previous level dimensions:
        // B * W(L-1) * H(L-1) * (2^2(L-1) + 2^2(L-2) + ... + 2^2(L-L)) / 8
        const u16 prevWidth = (width >> (level - 1));
        const u16 prevHeight = (height >> (level - 1));
        const size_t bpp = GLASS_getTexBitsPerPixel(format, type);
        return ((bpp * prevWidth * prevHeight * (((1u << (level << 1)) - 1) & 0x55555)) >> 3);
    }

    return 0;
}

size_t GLASS_tex_getSize(u16 width, u16 height, GLenum format, GLenum type, size_t level) {
    return (((width >> level) * (height >> level) * GLASS_getTexBitsPerPixel(format, type)) >> 3);
}

static size_t GLASS_dimLevels(size_t dim) {
    size_t n = 1;
    while (dim > 8) {
        dim >>= 1;
        ++n;
    }
    return n;
}

static size_t GLASS_calculateTexLevels(GLsizei width, GLsizei height) {
    const size_t widthLevels = GLASS_dimLevels(width);
    const size_t heightLevels = GLASS_dimLevels(height);
    return MIN(widthLevels, heightLevels);
}

size_t GLASS_tex_getAllocSize(u16 width, u16 height, GLenum format, GLenum type) {
    return GLASS_tex_getOffset(width, height, format, type, GLASS_calculateTexLevels(width, height));
}

void GLASS_tex_set(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram, u8** faces) {
    ASSERT(tex);
    ASSERT(faces);
    ASSERT(tex->flags & TEXTURE_FLAG_BOUND);

    const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
    const bool hadVRAM = tex->flags & TEXTURE_FLAG_VRAM;
    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = tex->faces[i];
        hadVRAM ? glassVRAMFree(p) : glassLinearFree(p);
        tex->faces[i] = faces[i];
    }

    tex->width = width;
    tex->height = height;
    tex->format = format;
    tex->dataType = type;

    if (vram) {
        tex->flags |= TEXTURE_FLAG_VRAM;
    } else {
        tex->flags &= ~(TEXTURE_FLAG_VRAM);
    }

    tex->flags |= TEXTURE_FLAG_INITIALIZED;
}

bool GLASS_tex_realloc(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram) {
    ASSERT(tex);
    ASSERT(tex->flags & TEXTURE_FLAG_BOUND);

    const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
    const size_t allocSize = GLASS_tex_getAllocSize(width, height, format, type);
    ASSERT(allocSize);

    u8* faces[GLASS_NUM_TEX_FACES];
    memset(faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));

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

    GLASS_tex_set(tex, width, height, format, type, vram, faces);
    return true;
}

TexStatus GLASS_tex_reallocIfNeeded(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram) {
    ASSERT(tex);
    ASSERT(tex->flags & TEXTURE_FLAG_BOUND);

    const bool hadVRAM = tex->flags & TEXTURE_FLAG_VRAM;
    if ((tex->width == width) && (tex->height == height) && (tex->format == format) && (tex->dataType == type) && (hadVRAM == vram))
        return TexStatus_Unchanged;

    return GLASS_tex_realloc(tex, width, height, format, type, vram) ? TexStatus_Updated : TexStatus_Failed;
}

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level) {
    ASSERT(tex);
    ASSERT(tex->flags & TEXTURE_FLAG_BOUND);
    ASSERT(face < GLASS_tex_getNumFaces(tex->target));

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t mipmapOffset = GLASS_tex_getOffset(tex->width, tex->height, tex->format, tex->type, level);
    u8* dest = tex->faces[face] + mipmapOffset;

    if (tex->flags & TEXTURE_FLAG_VRAM) {
        ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache(data, size)));
        GLASS_gx_copyTexture((u32)data, (u32)dest, size);
    } else {
        memcpy(dest, data, size);

        if (!ctx->initParams.flushAllLinearMem) {
            ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache(dest, size)));
        }
    }
}