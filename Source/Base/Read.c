/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Utility.h>
#include <RIP/Convert.h>
#include <RIP/Pixels.h>
#include <RIP/Tiling.h>

#include "Base/Math.h"
#include "Base/Read.h"
#include "Base/TexManager.h"

static inline size_t getPixelSize(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return 32;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
            return 16;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline RIPPixelFormat getPixelFormat(GLenum format) {
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
    }

    return 0;
}

void GLASS_read_colorBuffer(const FramebufferInfo* fb, GLint x, GLint y, size_t width, size_t height, RIPPixelFormat pixelFormat, u8* out) {
    KYGX_ASSERT(fb);
    KYGX_ASSERT(out);

    // Get color buffer. Width and height are swapped to account for the rotated screens.
    u8* cbAddr = NULL;
    u16 cbWidth = 0;
    u16 cbHeight = 0;
    GLenum cbFormat = 0;

    if (GLASS_OBJ_IS_RENDERBUFFER(fb->colorBuffer)) {
        RenderbufferInfo* cb = (RenderbufferInfo*)fb->colorBuffer;
        cbAddr = cb->address;
        cbWidth = cb->height;
        cbHeight = cb->width;
        cbFormat = cb->format;
    } else if (GLASS_OBJ_IS_TEXTURE(fb->colorBuffer)) {
        RenderbufferInfo cb;
        TextureInfo* tex = (TextureInfo*)fb->colorBuffer;
        GLASS_tex_getAsRenderbuffer(tex, fb->texFace, &cb);
        cbAddr = cb.address;
        cbWidth = cb.height;
        cbHeight = cb.width;
        cbFormat = cb.format;
    } else {
        KYGX_UNREACHABLE("Invalid color buffer!");
    }

    // Prepare coordinates.
    const u16 x0 = GLASS_MAX(0, x);
    const u16 y0 = GLASS_MAX(0, y);
    const s16 x1 = GLASS_MIN(cbHeight, x + width);
    const s16 y1 = GLASS_MIN(cbWidth, y + height);

    if (x0 >= x1 || y0 >= y1)
        return;

    const size_t clippedWidth = x1 - x0;
    const size_t clippedHeight = y1 - y0;

    // Read color buffer.
    const size_t srcPixelSize = getPixelSize(cbFormat);
    const size_t dstPixelSize = ripGetPixelFormatBPP(pixelFormat) >> 3;
    void* tmpBuffer = glassLinearAlloc(cbWidth * cbHeight * GLASS_MAX(srcPixelSize, dstPixelSize));
    KYGX_ASSERT(tmpBuffer);

    const RIPPixelFormat cbPixelFormat = getPixelFormat(cbFormat);
    KYGX_ASSERT(ripTilingCanUseHW(cbWidth, cbHeight, cbPixelFormat));

    bool ret = ripUntile(cbAddr, tmpBuffer, cbWidth, cbHeight, cbPixelFormat);
    KYGX_ASSERT(ret);

    ret = ripSwapPixelBytes(tmpBuffer, tmpBuffer, cbWidth, cbHeight, cbPixelFormat, false);
    KYGX_ASSERT(ret);

    // Do pixel conversion.
    ripConvertPixels(tmpBuffer, tmpBuffer, cbWidth, cbHeight, cbPixelFormat, pixelFormat);

    // Read lines.
    // Skip x > 0 lines, y > 0 pixels.
    const u8* src = &((const u8*)tmpBuffer)[((cbWidth * x0) + y0) * dstPixelSize];

    // Adjust the output buffer in case we read gibberish (coords < 0).
    const size_t dstOffsetX = x0 - x; // -x if x < 0 else 0 
    const size_t dstOffsetY = y0 - y; // -y if y < 0 else 0
    u8* dst = &out[((width * dstOffsetY) + dstOffsetX) * dstPixelSize];

    for (size_t r = 0; r < clippedHeight; ++r) {
        for (size_t c = 0; c < clippedWidth; ++c) {
            memcpy(&dst[(((r + dstOffsetY) * width) + c + dstOffsetX) * dstPixelSize], &src[((c * cbWidth) + r) * dstPixelSize], dstPixelSize);
        }
    }

    glassLinearFree(tmpBuffer);
}