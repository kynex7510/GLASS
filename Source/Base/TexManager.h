/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _GLASS_TEXMANAGER_H
#define _GLASS_TEXMANAGER_H

#include "Base/Types.h"

typedef enum {
    TEXREALLOCSTATUS_FAILED,
    TEXREALLOCSTATUS_UNCHANGED,
    TEXREALLOCSTATUS_UPDATED,
} TexReallocStatus;

void GLASS_tex_getAsRenderbuffer(const TextureInfo* tex, size_t face, RenderbufferInfo* out);

void GLASS_tex_setParams(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram, u8** faces);
TexReallocStatus GLASS_tex_realloc(TextureInfo* tex, size_t width, size_t height, GPUTexFormat format, bool vram);

void GLASS_tex_write(TextureInfo* tex, const u8* data, size_t face, size_t level);
void GLASS_tex_writeUntiled(TextureInfo* tex, const u8* data, size_t face, size_t level);

void GLASS_tex_readRect(TextureInfo* tex, u8* dst, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height);
void GLASS_tex_writeRect(TextureInfo* tex, const u8* data, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height);

void GLASS_tex_readUntiledRect(TextureInfo* tex, u8* dst, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height);
void GLASS_tex_writeUntiledRect(TextureInfo* tex, const u8* data, size_t face, size_t level, size_t x, size_t y, size_t width, size_t height);

#endif /* _GLASS_TEXMANAGER_H */