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
    u16 width;                     // Texture width.
    u16 height;                    // Texture height.
    GLenum format;                 // Texture format.
    GLenum dataType;               // Texture data type.
    size_t levels;                 // MipMap levels.
    size_t numOfSubTextures;       // Number of sub-textures.
    glassSubTexture subTextures[]; // Sub-texture info.
} glassTexture;

GLenum glassLoadTexture(const u8* data, size_t size, glassTexture** out);
GLenum glassLoadTextureFromFile(FILE* f, glassTexture** out);
GLenum glassLoadTextureFromPath(const char* path, glassTexture** out);

GLenum glassLoadCubeMap(const u8* data, size_t size, glassTexture** out);
GLenum glassLoadCubeMapFromFile(FILE* f, glassTexture** out);
GLenum glassLoadCubeMapFromPath(const char* path, glassTexture** out);

void glassDestroyTexture(glassTexture* tex);

const size_t glassGetTextureSize(const glassTexture* tex, size_t level);
const u8* glassGetTextureData(const glassTexture* tex, size_t level);
const u8* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level);
bool glassIsTextureCompressed(const glassTexture* tex);

#endif /* _GLASS_TEX3DS_H */