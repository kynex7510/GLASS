#ifndef _GLASS_TEX3DS_H
#define _GLASS_TEX3DS_H

#include <3ds.h>
#include <stdio.h>

#include "GLASS/Defs.h"

// Sub-texture object.
typedef struct {
    GLfloat xFactor; // Sub-texture X factor.
    GLfloat yFactor; // Sub-texture Y factor.
    GLsizei width;   // Sub-texture width.
    GLsizei height;  // Sub-texture height.
} glassSubTexture;

// Texture object.
typedef struct {
    u8* faces[6];                 // Internal texture data.
    bool isCubeMap;               // Whether this texture is a cube map.
    u16 width;                    // Texture width.
    u16 height;                   // Texture height.
    GLenum format;                // Texture format.
    GLenum type;                  // Texture type.
    size_t levels;                // MipMap levels.
    size_t numOfSubTextures;      // Number of sub-textures.
    glassSubTexture* subTextures; // Sub-texture info.
} glassTexture;

void glassLoadTexture(const u8* data, size_t size, glassTexture* out);
void glassLoadTextureFromFile(FILE* f, glassTexture* out);
void glassLoadTextureFromPath(const char* path, glassTexture* out);

void glassMoveTextureData(glassTexture* tex);
void glassDestroyTexture(glassTexture* tex);

const size_t glassGetTextureSize(const glassTexture* tex, size_t level);
const u8* glassGetTextureData(const glassTexture* tex, size_t face, size_t level);
const u8* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level);
bool glassIsTextureCompressed(const glassTexture* tex);

#endif /* _GLASS_TEX3DS_H */