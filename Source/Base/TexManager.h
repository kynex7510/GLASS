#ifndef _GLASS_TEXMANAGER_H
#define _GLASS_TEXMANAGER_H

#include "Base/Types.h"

typedef enum {
    TEXREALLOCSTATUS_FAILED,
    TEXREALLOCSTATUS_UNCHANGED,
    TEXREALLOCSTATUS_UPDATED,
} TexReallocStatus;

void GLASS_tex_setParams(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram, u8** faces);
TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram);

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t size, size_t face, size_t level);
void GLASS_tex_writeUntiled(TextureInfo* tex, const u8* data, size_t face, size_t level);

#endif /* _GLASS_TEXMANAGER_H */