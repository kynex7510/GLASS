#ifndef _GLASS_TEXCOMMON_H
#define _GLASS_TEXCOMMON_H

#include "Base/Types.h"

typedef enum {
    TexReallocStatus_Failed,
    TexReallocStatus_Unchanged,
    TexReallocStatus_Updated,
} TexReallocStatus;

size_t GLASS_tex_bpp(GLenum format, GLenum type);
GPU_TEXCOLOR GLASS_tex_unwrapFormat(GLenum format, GLenum type);
GLenum GLASS_tex_wrapFormat(GPU_TEXCOLOR format);
GLenum GLASS_tex_wrapType(GPU_TEXCOLOR format);
bool GLASS_tex_isFormatValid(GLenum format, GLenum type);
bool GLASS_tex_isCompressed(GLenum format);

size_t GLASS_tex_getNumFaces(GLenum target);
size_t GLASS_tex_getOffset(size_t width, size_t height, GLenum format, GLenum type, size_t level);
size_t GLASS_tex_getSize(size_t width, size_t height, GLenum format, GLenum type, size_t level);
size_t GLASS_tex_getAllocSize(size_t width, size_t height, GLenum format, GLenum type);

void GLASS_tex_set(TextureInfo* tex, size_t width, size_t height, GLenum format, GLenum type, bool vram, u8** faces);
TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, GLenum format, GLenum type, bool vram);

void GLASS_tex_flip(const u8* src, u8* dst, size_t width, size_t height, size_t bpp);

void GLASS_tex_makeTiled(const u8* src, u8* dst, size_t width, size_t height, GLenum format, GLenum type);
void GLASS_tex_makeLinear(const u8* src, u8* dst, size_t width, size_t height, GLenum format, GLenum type);

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level);
void GLASS_tex_writeRaw(TextureInfo* tex, const u8* data, size_t face, size_t level);

#endif /* _GLASS_TEXCOMMON_H */