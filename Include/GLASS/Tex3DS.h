#ifndef _GLASS_TEX3DS_H
#define _GLASS_TEX3DS_H

#include <GLASS/Defs.h>

#ifndef GLASS_BAREMETAL
#include <stdio.h>
#endif // !GLASS_BAREMETAL

// Sub-texture object.
typedef struct {
    GLfloat xFactor; // Sub-texture X factor.
    GLfloat yFactor; // Sub-texture Y factor.
    uint16_t width;  // Sub-texture width.
    uint16_t height; // Sub-texture height.
} glassSubTexture;

// Pixel format object.
typedef struct {
    GLenum format; // Pixel format.
    GLenum type;   // Pixel type.
} glassPixelFormat;

// Texture object.
typedef struct {
    uint8_t* faces[6];            // Internal texture data.
    bool isCubeMap;               // Whether this texture is a cube map.
    uint16_t width;               // Texture width.
    uint16_t height;              // Texture height.
    glassPixelFormat pixelFormat; // Pixel format.
    size_t levels;                // MipMap levels.
    size_t numOfSubTextures;      // Number of sub-textures.
    glassSubTexture* subTextures; // Sub-texture info.
} glassTexture;

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

void glassLoadTexture(const uint8_t* data, size_t size, glassTexture* out);

#ifndef GLASS_BAREMETAL
void glassLoadTextureFromFile(FILE* f, glassTexture* out);
void glassLoadTextureFromPath(const char* path, glassTexture* out);
#endif // !GLASS_BAREMETAL

void glassMoveTextureData(glassTexture* tex);
void glassDestroyTexture(glassTexture* tex);

const size_t glassGetTextureSize(const glassTexture* tex, size_t level);
const uint8_t* glassGetTextureData(const glassTexture* tex, size_t face, size_t level);
const uint8_t* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level);

bool glassIsCompressed(const glassPixelFormat* pixelFormat);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_TEX3DS_H */