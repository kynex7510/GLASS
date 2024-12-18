#ifndef _GLASS_BASE_PIXELS_H
#define _GLASS_BASE_PIXELS_H

#include "Base/Types.h"

void GLASS_pixels_flip(const u8* src, u8* dst, size_t width, size_t height, size_t bpp);
void GLASS_pixels_tiling(const u8* src, u8* dst, size_t width, size_t height, GLenum format, GLenum type, bool makeTiled);

#endif /* _GLASS_BASE_PIXELS_H */