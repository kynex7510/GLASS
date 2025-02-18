#include "Base/Pixels.h"
#include "Base/Utility.h"
#include "Platform/GX.h"

GPU_TEXCOLOR GLASS_pixels_tryUnwrapTexFormat(const glassPixelFormat* pixelFormat) {
    ASSERT(pixelFormat);

    if (pixelFormat->type == GL_RENDERBUFFER) {
        switch (pixelFormat->format) {
            case GL_RGBA8_OES:
                return GPU_RGBA8;
            case GL_RGB8_OES:
                return GPU_RGB8;
            case GL_RGB565:
                return GPU_RGB565;
            case GL_RGB5_A1:
                return GPU_RGBA5551;
            case GL_RGBA4:
                return GPU_RGBA4;
        }
    }

    if (pixelFormat->format == GL_ALPHA) {
        switch (pixelFormat->type) {
            case GL_UNSIGNED_BYTE:
                return GPU_A8;
            case GL_UNSIGNED_NIBBLE_PICA:
                return GPU_A4;
        }
    }

    if (pixelFormat->format == GL_LUMINANCE) {
        switch (pixelFormat->type) {
            case GL_UNSIGNED_BYTE:
                return GPU_L8;
            case GL_UNSIGNED_NIBBLE_PICA:
                return GPU_L4;
        }
    }

    if (pixelFormat->format == GL_LUMINANCE_ALPHA) {
        switch (pixelFormat->type) {
            case GL_UNSIGNED_BYTE:
                return GPU_LA8;
            case GL_UNSIGNED_BYTE_4_4_PICA:
                return GPU_LA4;
        }
    }

    if (pixelFormat->format == GL_RGB) {
        switch (pixelFormat->type) {
            case GL_UNSIGNED_BYTE:
                return GPU_RGB8;
            case GL_UNSIGNED_SHORT_5_6_5:
                return GPU_RGB565;
        }
    }

    if (pixelFormat->format == GL_RGBA) {
        switch (pixelFormat->type) {
            case GL_UNSIGNED_BYTE:
                return GPU_RGBA8;
            case GL_UNSIGNED_SHORT_5_5_5_1:
                return GPU_RGBA5551;
            case GL_UNSIGNED_SHORT_4_4_4_4:
                return GPU_RGBA4;
        }
    }

    if (pixelFormat->format == GL_HILO8_PICA && pixelFormat->type == GL_UNSIGNED_BYTE)
        return GPU_HILO8;

    if (pixelFormat->format == GL_ETC1_RGB8_OES)
        return GPU_ETC1;

    if (pixelFormat->format == GL_ETC1_ALPHA_RGB8_A4_PICA)
        return GPU_ETC1A4;

    return GLASS_INVALID_TEX_FORMAT;
}

u32 GLASS_pixels_tryUnwrapTransferFormat(const glassPixelFormat* pixelFormat) {
    ASSERT(pixelFormat);

    switch (GLASS_pixels_tryUnwrapTexFormat(pixelFormat)) {
        case GPU_RGBA8:
            return GLASS_GX_TRANSFER_FLAG_FMT_RGBA8;
        case GPU_RGB8:
            return GLASS_GX_TRANSFER_FLAG_FMT_RGB8;
        case GPU_RGB565:
            return GLASS_GX_TRANSFER_FLAG_FMT_RGB565;
        case GPU_RGBA5551:
            return GLASS_GX_TRANSFER_FLAG_FMT_RGB5A1;
        case GPU_RGBA4:
            return GLASS_GX_TRANSFER_FLAG_FMT_RGBA4;
        default:;
    }

    return GLASS_INVALID_TRANSFER_FORMAT;
}

static size_t GLASS_renderbufferBpp(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 32;
        case GL_RGB8_OES:
        case GL_DEPTH_COMPONENT24_OES:
            return 24;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
        case GL_DEPTH_COMPONENT16:
            return 16;
    }

    UNREACHABLE("Invalid parameter!");
}

static size_t GLASS_texBpp(GPU_TEXCOLOR format) {
    switch (format) {
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

size_t GLASS_pixels_bpp(const glassPixelFormat* pixelFormat) {
    ASSERT(pixelFormat);

    if (pixelFormat->type == GL_RENDERBUFFER)
        return GLASS_renderbufferBpp(pixelFormat->format);

    return GLASS_texBpp(GLASS_pixels_tryUnwrapTexFormat(pixelFormat));
}