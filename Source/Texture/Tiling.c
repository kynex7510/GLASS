#include "Texture/Texture.h"
#include "Base/Utility.h"
#include "Base/GX.h"

#include <string.h> // memcpy

static GLenum GLASS_getHwTilingFormat(GLenum format, GLenum type) {
    if (format == GL_RGBA) {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGBA8_OES;

        if (type == GL_UNSIGNED_SHORT_4_4_4_4)
            return GL_RGBA4;
    }

    if (format == GL_RGB) {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGB8_OES;

        if (type == GL_UNSIGNED_SHORT_5_6_5)
            return GL_RGB565;

        if (type == GL_UNSIGNED_SHORT_5_5_5_1)
            return GL_RGB5_A1;
    }

    return 0;
}

static void GLASS_handleTile(const u8* src, u8* dst, size_t width, size_t bpp, bool makeTiled) {
    // https://en.wikipedia.org/wiki/Moser-de_Bruijn_sequence
    static const u8 Z_ORDER_TABLE[8] = { 0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15 };

    for (size_t r = 0; r < 8; ++r) {
        // Optimization: pick 2 pixels at time since they're always contiguous.
        for (size_t c = 0; c < 8; c += 2) {
            const size_t linearIndex = (((r * width) + c) * bpp) >> 3;
            const size_t tiledIndex = ((Z_ORDER_TABLE[c] + (Z_ORDER_TABLE[r] << 1)) * bpp) >> 3;
            memcpy(&dst[makeTiled ? tiledIndex : linearIndex], &src[makeTiled ? linearIndex : tiledIndex], bpp >> 2);
        }
    }
}

static void GLASS_swTiling(const u8* src, u8* dst, size_t width, size_t height, size_t bpp, bool makeTiled) {
    size_t tiledIndex = 0;
    for (size_t r = 0; r < height; r += 8) {
        for (size_t c = 0; c < width; c += 8) {
            const size_t linearIndex = (((r * width) + c) * bpp) >> 3;
            GLASS_handleTile(&src[makeTiled ? linearIndex : tiledIndex], &dst[makeTiled ? tiledIndex : linearIndex], width, bpp, makeTiled);
            tiledIndex += (64 * bpp) >> 3;
        }
    }
}

void GLASS_tilingImpl(u32 srcAddr, u32 dstAddr, GLsizei width, GLsizei height, GLenum format, GLenum type, bool makeTiled) {
    // Use the hardware if possible.
    const GLenum tilingFormat = GLASS_getHwTilingFormat(format, type);
    if (tilingFormat) {
        TexTransformationParams params;
        params.inputWidth = width;
        params.inputHeight = height;
        params.inputFormat = tilingFormat;
        params.outputWidth = params.inputWidth;
        params.outputHeight = params.inputHeight;
        params.outputFormat = params.inputFormat;
        params.verticalFlip = false;
        params.makeTiled = true;
        GLASS_gx_transformTexture(srcAddr, dstAddr, &params);
    } else {
        ASSERT(!glassIsVRAM((void*)srcAddr));
        ASSERT(!glassIsVRAM((void*)dstAddr));
        GLASS_swTiling((u8*)srcAddr, (u8*)dstAddr, width, height, GLASS_tex_bpp(format, type), makeTiled);
    }
}

void GLASS_tex_makeTiled(u32 srcAddr, u32 dstAddr, GLsizei width, GLsizei height, GLenum format, GLenum type) {
    GLASS_tilingImpl(srcAddr, dstAddr, width, height, format, type, true);
}

void GLASS_tex_makeLinear(u32 srcAddr, u32 dstAddr, GLsizei width, GLsizei height, GLenum format, GLenum type) {
    GLASS_tilingImpl(srcAddr, dstAddr, width, height, format, type, false);
}