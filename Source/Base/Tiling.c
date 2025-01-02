#include "Base/Pixels.h"
#include "Platform/GX.h"
#include "Base/Utility.h"
#include "Base/Texture.h"

#include <string.h> // memcpy

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

static GLenum GLASS_supportHwTiling(size_t width, size_t height, GLenum format, GLenum type) {
    // Transfer engine doesn't support anything lower than 64.
    if ((width >= 64) && (height >= 64)) {
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
    }

    return 0;
}

void GLASS_pixels_tiling(const u8* src, u8* dst, size_t width, size_t height, GLenum format, GLenum type, bool makeTiled) {
    ASSERT(glassIsLinear(src));
    ASSERT(glassIsLinear(dst));
    ASSERT(width >= 8);
    ASSERT(height >= 8);
    ASSERT(!(width & 8));
    ASSERT(!(height & 8));

    // Use the hardware if possible.
    const GLenum hwTilingFormat = GLASS_supportHwTiling(width, height, format, type);
    if (hwTilingFormat) {
        GLASS_gx_tiling((u32)src, (u32)dst, width, height, hwTilingFormat, makeTiled);
    } else {
        GLASS_swTiling(src, dst, width, height, GLASS_tex_bpp(format, type), makeTiled);
    }
}