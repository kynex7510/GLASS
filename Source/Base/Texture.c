#include "Base/Texture.h"
#include "Platform/GX.h"
#include "Base/Utility.h"
#include "Base/Pixels.h"

#include <string.h> // memset

size_t GLASS_tex_bpp(GLenum format, GLenum type) {
    switch (GLASS_tex_unwrapFormat(format, type)) {
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

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEXCOLOR GLASS_unwrapTexFormatImpl(GLenum format, GLenum dataType) {
    if (format == GL_ALPHA) {
        switch (dataType) {
            case GL_UNSIGNED_BYTE:
                return GPU_A8;
            case GL_UNSIGNED_NIBBLE_PICA:
                return GPU_A4;
        }
    }

    if (format == GL_LUMINANCE) {
        switch (dataType) {
            case GL_UNSIGNED_BYTE:
                return GPU_L8;
            case GL_UNSIGNED_NIBBLE_PICA:
                return GPU_L4;
        }
    }

    if (format == GL_LUMINANCE_ALPHA) {
        switch (dataType) {
            case GL_UNSIGNED_BYTE:
                return GPU_LA8;
            case GL_UNSIGNED_BYTE_4_4_PICA:
                return GPU_LA4;
        }
    }

    if (format == GL_RGB) {
        switch (dataType) {
            case GL_UNSIGNED_BYTE:
                return GPU_RGB8;
            case GL_UNSIGNED_SHORT_5_6_5:
                return GPU_RGB565;
        }
    }

    if (format == GL_RGBA) {
        switch (dataType) {
            case GL_UNSIGNED_BYTE:
                return GPU_RGBA8;
            case GL_UNSIGNED_SHORT_5_5_5_1:
                return GPU_RGBA5551;
            case GL_UNSIGNED_SHORT_4_4_4_4:
                return GPU_RGBA4;
        }
    }

    if (format == GL_HILO8_PICA && dataType == GL_UNSIGNED_BYTE)
        return GPU_HILO8;

    if (format == GL_ETC1_RGB8_OES)
        return GPU_ETC1;

    if (format == GL_ETC1_ALPHA_RGB8_A4_PICA)
        return GPU_ETC1A4;

    return (GPU_TEXCOLOR)-1;
}

GPU_TEXCOLOR GLASS_tex_unwrapFormat(GLenum format, GLenum type) {
    GPU_TEXCOLOR texFmt = GLASS_unwrapTexFormatImpl(format, type);
    if (texFmt != (GPU_TEXCOLOR)-1)
        return texFmt;

    UNREACHABLE("Invalid parameter!");
}

GLenum GLASS_tex_wrapFormat(GPU_TEXCOLOR format) {
    switch (format) {
        case GPU_A8:
        case GPU_A4:
            return GL_ALPHA;
        case GPU_L8:
        case GPU_L4:
            return GL_LUMINANCE;
        case GPU_LA8:
        case GPU_LA4:
            return GL_LUMINANCE_ALPHA;
        case GPU_RGB8:
        case GPU_RGB565:
            return GL_RGB;
        case GPU_RGBA8:
        case GPU_RGBA5551:
        case GPU_RGBA4:
            return GL_RGBA;
        case GPU_HILO8:
            return GL_HILO8_PICA;
            break;
        case GPU_ETC1:
            return GL_ETC1_RGB8_OES;
        case GPU_ETC1A4:
            return GL_ETC1_ALPHA_RGB8_A4_PICA;
    }

    UNREACHABLE("Invalid parameter!");
}

GLenum GLASS_tex_wrapType(GPU_TEXCOLOR format) {
    switch (format) {
        case GPU_A8:
        case GPU_L8:
        case GPU_LA8:
        case GPU_RGB8:
        case GPU_RGBA8:
        case GPU_HILO8:
            return GL_UNSIGNED_BYTE;
        case GPU_RGB565:
            return GL_UNSIGNED_SHORT_5_6_5;
        case GPU_RGBA4:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case GPU_RGBA5551:
            return GL_UNSIGNED_SHORT_5_5_5_1;
        case GPU_A4:
        case GPU_L4:
            return GL_UNSIGNED_NIBBLE_PICA;
        case GPU_LA4:
            return GL_UNSIGNED_BYTE_4_4_PICA;
        // Compressed texture don't have a data type.
        case GPU_ETC1:
        case GPU_ETC1A4:
            return 0;
    }

    UNREACHABLE("Invalid parameter!");
}

bool GLASS_tex_isFormatValid(GLenum format, GLenum type) { return GLASS_unwrapTexFormatImpl(format, type) != (GPU_TEXCOLOR)-1; }
bool GLASS_tex_isCompressed(GLenum format) { return (format == GL_ETC1_RGB8_OES) || (format == GL_ETC1_ALPHA_RGB8_A4_PICA); }

size_t GLASS_tex_getNumFaces(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return 1;
        case GL_TEXTURE_CUBE_MAP:
            return 6;
    }

    UNREACHABLE("Invalid parameter!");
}

size_t GLASS_tex_getOffset(size_t width, size_t height, GLenum format, GLenum type, size_t level) {
    ASSERT(level <= GLASS_NUM_TEX_LEVELS);

    if (level > 0) {
        // Basically rewrite the offset in terms of the previous level dimensions:
        // B * W(L-1) * H(L-1) * (2^2(L-1) + 2^2(L-2) + ... + 2^2(L-L)) / 8
        const u16 prevWidth = (width >> (level - 1));
        const u16 prevHeight = (height >> (level - 1));
        const size_t bpp = GLASS_tex_bpp(format, type);
        return ((bpp * prevWidth * prevHeight * (((1u << (level << 1)) - 1) & 0x55555)) >> 3);
    }

    return 0;
}

size_t GLASS_tex_getSize(size_t width, size_t height, GLenum format, GLenum type, size_t level) {
    return (((width >> level) * (height >> level) * GLASS_tex_bpp(format, type)) >> 3);
}

static size_t GLASS_numTexLevels(GLsizei width, GLsizei height) {
    return 29 - __builtin_clz(MAX(width, height));
}

size_t GLASS_tex_getAllocSize(size_t width, size_t height, GLenum format, GLenum type, size_t levels) {
    if (levels == -1)
        levels = GLASS_numTexLevels(width, height);

    return GLASS_tex_getOffset(width, height, format, type, levels);
}

void GLASS_tex_set(TextureInfo* tex, size_t width, size_t height, GLenum format, GLenum type, bool vram, u8** faces) {
    ASSERT(tex);
    ASSERT(faces);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(GLASS_utility_isPowerOf2(width));
    ASSERT(GLASS_utility_isPowerOf2(height));

    const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = tex->faces[i];
        tex->vram ? glassVRAMFree(p) : glassLinearFree(p);
        tex->faces[i] = faces[i];
    }

    tex->width = width;
    tex->height = height;
    tex->format = format;
    tex->type = type;
    tex->vram = vram;
}

bool GLASS_reallocTexImpl(TextureInfo* tex, size_t width, size_t height, GLenum format, GLenum type, bool vram) {
    u8* faces[GLASS_NUM_TEX_FACES];
    memset(faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));

    if (width && height) {
        const size_t numFaces = GLASS_tex_getNumFaces(tex->target);
        const size_t allocSize = GLASS_tex_getAllocSize(width, height, format, type, -1);

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

    GLASS_tex_set(tex, width, height, format, type, vram, faces);
    return true;
}

TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, GLenum format, GLenum type, bool vram) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(GLASS_utility_isPowerOf2(width));
    ASSERT(GLASS_utility_isPowerOf2(height));

    if ((tex->width == width) && (tex->height == height) && (tex->format == format) && (tex->type == type) && (tex->vram == vram))
        return TexReallocStatus_Unchanged;

    return GLASS_reallocTexImpl(tex, width, height, format, type, vram) ? TexReallocStatus_Updated : TexReallocStatus_Failed;
}

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);
    ASSERT(glassIsLinear(data));

    const size_t mipmapOffset = GLASS_tex_getOffset(tex->width, tex->height, tex->format, tex->type, level);
    u8* dst = tex->faces[face] + mipmapOffset;

    if (!size)
        size = GLASS_tex_getSize(tex->width, tex->height, tex->format, tex->type, level);

    GLASS_gx_copyTexture((u32)data, (u32)dst, size, size, 1);
}

void GLASS_tex_writeRaw(TextureInfo* tex, const u8* data, size_t face, size_t level) {
    ASSERT(tex);
    ASSERT(tex->target != GLASS_TEX_TARGET_UNBOUND);

    const size_t width = tex->width >> level;
    const size_t height = tex->height >> level;
    const size_t bpp = GLASS_tex_bpp(tex->format, tex->type);

    u8* flipped = glassLinearAlloc((width * height * bpp) >> 3);
    ASSERT(flipped);
    GLASS_pixels_flip(data, flipped, width, height, bpp);

    u8* tiled = glassLinearAlloc((width * height * bpp) >> 3);
    ASSERT(tiled);
    GLASS_pixels_tiling(flipped, tiled, width, height, tex->format, tex->type, true);
    glassLinearFree(flipped);

    GLASS_tex_write(tex, tiled, 0, face, level);
    glassLinearFree(tiled);
}