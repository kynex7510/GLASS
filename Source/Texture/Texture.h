#ifndef _GLASS_TEXCOMMON_H
#define _GLASS_TEXCOMMON_H

#include "Base/Types.h"

typedef enum {
    TexStatus_Failed,
    TexStatus_Unchanged,
    TexStatus_Updated,
} TexStatus;

size_t GLASS_tex_bpp(GLenum format, GLenum type);
GPU_TEXCOLOR GLASS_tex_unwrapFormat(GLenum format, GLenum type);
GLenum GLASS_tex_wrapFormat(GPU_TEXCOLOR format);
GLenum GLASS_tex_wrapType(GPU_TEXCOLOR format);
bool GLASS_tex_isFormatValid(GLenum format, GLenum type);

size_t GLASS_tex_getNumFaces(GLenum target);
size_t GLASS_tex_getOffset(u16 width, u16 height, GLenum format, GLenum type, size_t level);
size_t GLASS_tex_getSize(u16 width, u16 height, GLenum format, GLenum type, size_t level);
size_t GLASS_tex_getAllocSize(u16 width, u16 height, GLenum format, GLenum type);

void GLASS_tex_set(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram, u8** faces);
bool GLASS_tex_realloc(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram);
TexStatus GLASS_tex_reallocIfNeeded(TextureInfo* tex, GLsizei width, GLsizei height, GLenum format, GLenum type, bool vram);

void GLASS_tex_makeTiled(u32 srcAddr, u32 dstAddr, GLsizei width, GLsizei height, GLenum format, GLenum type);
void GLASS_tex_makeLinear(u32 srcAddr, u32 dstAddr, GLsizei width, GLsizei height, GLenum format, GLenum type);
void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level, bool makeTiled);

#endif /* _GLASS_TEXCOMMON_H */