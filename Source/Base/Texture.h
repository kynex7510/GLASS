#ifndef _GLASS_TEXCOMMON_H
#define _GLASS_TEXCOMMON_H

#include "Base/Types.h"

#define GLASS_TEX_INVALID_FORMAT (GPU_TEXCOLOR)(-1)

typedef enum {
    TexReallocStatus_Failed,
    TexReallocStatus_Unchanged,
    TexReallocStatus_Updated,
} TexReallocStatus;

size_t GLASS_tex_getNumFaces(GLenum target);
size_t GLASS_tex_getOffset(size_t width, size_t height, const glassPixelFormat* pixelFormat, size_t level);
size_t GLASS_tex_getAllocSize(size_t width, size_t height, const glassPixelFormat* pixelFormat, size_t levels);

void GLASS_tex_set(TextureInfo* tex, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool vram, uint8_t** faces);
TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, const glassPixelFormat* pixelFormat, bool vram);

void GLASS_tex_write(TextureInfo* tex, const uint8_t* data, size_t size, size_t face, size_t level);
void GLASS_tex_writeRaw(TextureInfo* tex, const uint8_t* data, size_t face, size_t level);

#endif /* _GLASS_TEXCOMMON_H */