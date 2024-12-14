#include "Texture/Texture.h"
#include "Base/Utility.h"
#include "Base/GX.h"

#include <string.h>

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

    UNREACHABLE("Invalid GPU texture format!");
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

    UNREACHABLE("Invalid texture format!");
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

    UNREACHABLE("Invalid GPU texture format!");
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

    UNREACHABLE("Invalid GPU texture format!");
}

bool GLASS_tex_isFormatValid(GLenum format, GLenum type) { return GLASS_unwrapTexFormatImpl(format, type) != (GPU_TEXCOLOR)-1; }

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
        const size_t bpp = GLASS_tex_bpp(format, type);
        return ((bpp * prevWidth * prevHeight * (((1u << (level << 1)) - 1) & 0x55555)) >> 3);
    }

    return 0;
}

size_t GLASS_tex_getSize(u16 width, u16 height, GLenum format, GLenum type, size_t level) {
    return (((width >> level) * (height >> level) * GLASS_tex_bpp(format, type)) >> 3);
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

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level, bool makeTiled) {
    ASSERT(tex);
    ASSERT(tex->flags & TEXTURE_FLAG_BOUND);
    ASSERT(face < GLASS_tex_getNumFaces(tex->target));

    CtxCommon* ctx = GLASS_context_getCommon();

    const size_t mipmapOffset = GLASS_tex_getOffset(tex->width, tex->height, tex->format, tex->type, level);
    u8* dest = tex->faces[face] + mipmapOffset;

    const bool vram = tex->flags & TEXTURE_FLAG_VRAM;
    if (makeTiled) {
        GLASS_tex_makeTiled((u32)data, (u32)dest, tex->width, tex->height, tex->format, tex->dataType);
    } else {
        if (vram) {
            GLASS_gx_copyTexture((u32)data, (u32)dest, size);
        } else {
            memcpy(dest, data, size);
        }
    }

    if (!ctx->initParams.flushAllLinearMem) {
        ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache(dest, size)));
    }
}