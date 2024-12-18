#include "Base/Pixels.h"

#include <string.h> // memcpy

typedef void(*SwapBytes_t)(u8* buffer, size_t size);

static void GLASS_swapBytes4(u8* buffer, size_t size) {
    u32* p = (u32*)buffer;
    for (size_t i = 0; i < (size >> 2); ++i) {
        const u32 v = p[i];
        p[i] = ((v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24));
    }
}

static void GLASS_swapBytes3(u8* buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        const u8 tmp = buffer[i * 3];
        buffer[i * 3] = buffer[(i * 3) + 2];
        buffer[(i * 3) + 2] = tmp;
    }
}

static void GLASS_swapBytes2(u8* buffer, size_t size) {
    u16* p = (u16*)buffer;
    for (size_t i = 0; i < (size >> 1); ++i) {
        const u16 v = p[i];
        p[i] = ((v >> 8) | (v << 8));
    }
}

static SwapBytes_t GLASS_getSwapBytesFn(size_t bpp) {
    switch (bpp >> 3) {
        case 4:
            return GLASS_swapBytes4;
        case 3:
            return GLASS_swapBytes3;
        case 2:
            return GLASS_swapBytes2;
    }

    return NULL;
}

void GLASS_pixels_flip(const u8* src, u8* dst, size_t width, size_t height, size_t bpp) {
    const SwapBytes_t swapBytesFn = GLASS_getSwapBytesFn(bpp);
    const size_t rowSize = (width * bpp) >> 3;

    for (size_t r = 0; r < height; ++r) {
        const size_t srcIndex = r * rowSize;
        const size_t dstIndex = (height - r - 1) * rowSize;
        memcpy(&dst[dstIndex], &src[srcIndex], rowSize);

        if (swapBytesFn)
            swapBytesFn(&dst[dstIndex], rowSize);
    }
}