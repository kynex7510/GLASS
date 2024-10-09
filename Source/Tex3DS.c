// Heavily based on tex3ds.c from citro3d
// https://github.com/devkitPro/citro3d/blob/master/source/tex3ds.c

#include "Utility.h"

#include <string.h> // memcpy

typedef void(*TexReadCallback)(struct TexStream*, void* out, size_t offset, size_t size);

typedef struct {
    void* handle;
    size_t size;
    TexReadCallback read;
} TexStream;

typedef struct __attribute__((packed)) {
    u16 numSubTextures;
	u8 widthLog2 : 3;
	u8 heightLog2 : 3;
	u8 type : 1;
	u8 format;
	u8 mipmapLevels;
} RawHeader;

static GLenum GLASS_convertTexFormat(GPU_TEXCOLOR format) {
    switch (format) {
        case GPU_A8:
        case GPU_A4:
            return GL_ALPHA;
        case GPU_L8:
        case GPU_L4:
            return GL_LUMINANCE;
        case GPU_LA8:
        case GPU_LA4:
            return GL_LUMINANCE_ALPHA;
        case GPU_RGB8:
        case GPU_RGB565:
            return GL_RGB;
        case GPU_RGBA8:
        case GPU_RGBA5551:
        case GPU_RGBA4:
            return GL_RGBA;
        case GPU_HILO8:
            return GL_HILO8_PICA;
            break;
        case GPU_ETC1:
            return GL_ETC1_RGB8_OES;
        // TODO
        case GPU_ETC1A4:
            break;
    }

    UNREACHABLE("Invalid GPU texture format!");
}

static GLenum GLASS_getTexDataType(GPU_TEXCOLOR format) {
    switch (format) {
        case GPU_A8:
        case GPU_L8:
        case GPU_LA8:
        case GPU_RGB8:
        case GPU_RGBA8:
        case GPU_HILO8:
            return GL_UNSIGNED_BYTE;
        case GPU_RGB565:
            return GL_UNSIGNED_SHORT_5_6_5;
        case GPU_RGBA4:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case GPU_RGBA5551:
            return GL_UNSIGNED_SHORT_5_5_5_1;
        case GPU_A4:
        case GPU_L4:
            return GL_UNSIGNED_NIBBLE_PICA;
        case GPU_LA4:
            return GL_UNSIGNED_BYTE_4_4_PICA;
    }

    // Compressed textures don't have a data type.
    return 0;
}

static glassTexture* GLASS_loadTextureImpl(TexStream* stream) {
    // Read texture header.
    RawHeader header;
    stream->read(stream, &header, 0, sizeof(RawHeader));
    ASSERT(header.type == GPU_TEX_2D);

    // Create texture object.
    glassTexture* tex = glassVirtualAlloc(sizeof(glassTexture));
    if (!tex)
        return NULL;

    tex->width = (1 << (header.widthLog2 + 3));
    tex->height = (1 << (header.heightLog2 + 3));
    tex->format =  GLASS_convertTexFormat(header.format);
    tex->dataType = GLASS_getTexDataType(header.format);
    tex->levels = header.mipmapLevels;

    // Load texture data.
    // TODO
    UNREACHABLE("Unimplemented!");
}

static bool GLASS_loadCubeMapImpl(TexStream* stream, glassTexture** out) {
    ASSERT(out);
    UNREACHABLE("Unimplemented!");
}

static void GLASS_texStreamReadMem(TexStream* stream, void* out, size_t offset, size_t size) {
    ASSERT(stream);
    ASSERT(size < stream->size && (offset + size) < stream->size);
    memcpy(out, (u8*)(stream->handle) + offset, size);
}

static void GLASS_texStreamReadFile(TexStream* stream, void* out, size_t offset, size_t size) {
    ASSERT(stream);
    ASSERT(size < stream->size && (offset + size) < stream->size);
    ASSERT(fseek((FILE*)stream->handle, offset, SEEK_SET));
    ASSERT(fread(out, size, 1, (FILE*)stream->handle) == 1);
}

static INLINE void GLASS_getMemTexStream(TexStream* stream, const u8* data, size_t size) {
    stream->handle = (void*)data;
    stream->size = size;
    stream->read = GLASS_texStreamReadMem;
}

static INLINE void GLASS_getFileTexStream(TexStream* stream, FILE* f) {
    ASSERT(fseek(f, 0, SEEK_END) == 0);
    stream->handle = (void*)f;
    stream->size = ftell(f);
    stream->read = GLASS_texStreamReadFile;
}

glassTexture* glassLoadTexture(const u8* data, size_t size) {
    TexStream stream;
    GLASS_getMemTexStream(&stream, data, size);
    return GLASS_loadTextureImpl(&stream);
}

bool glassLoadCubeMap(const u8* data, size_t size, glassTexture** out) {
    TexStream stream;
    GLASS_getMemTexStream(&stream, data, size);
    return GLASS_loadCubeMapImpl(&stream, out);
}

glassTexture* glassLoadTextureFromFile(FILE* f) {
    TexStream stream;
    GLASS_getFileTexStream(&stream, f);
    return GLASS_loadTextureImpl(&stream);
}

bool glassLoadCubeMapFromFile(FILE* f, glassTexture** out) {
    TexStream stream;
    GLASS_getFileTexStream(&stream, f);
    return GLASS_loadCubeMapImpl(&stream, out);
}

glassTexture* glassLoadTextureFromPath(const char* path) {
    FILE* f = fopen(path, "rb");
    ASSERT(f);
    glassTexture* tex = glassLoadTextureFromFile(f);
    fclose(f);
    return tex;
}

bool glassLoadCubeMapFromPath(const char* path, glassTexture** out) {
    FILE* f = fopen(path, "rb");
    ASSERT(f);
    bool ret = glassLoadCubeMapFromFile(f, out);
    fclose(f);
    return ret;
}

void glassFreeTexture(glassTexture* tex) {
    ASSERT(tex);

    for (size_t i = 0; i < tex->levels; ++i)
        glassVirtualFree(tex->data[i]);

    glassVirtualFree(tex);
}