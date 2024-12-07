// Heavily based on tex3ds.c from citro3d
// https://github.com/devkitPro/citro3d/blob/master/source/tex3ds.c

#include "Utility.h"

#include <string.h> // memcpy

typedef struct {
    void* handle;
    size_t offset;
    size_t size;
    decompressCallback read;
} TexStream;

typedef struct __attribute__((packed)) {
    u16 numSubTextures;
    u8 widthLog2 : 3;
    u8 heightLog2 : 3;
    u8 type : 1;
    u8 format;
    u8 mipmapLevels;
} RawHeader;

typedef struct {
    u16 width;
    u16 height;
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
} RawSubTexture;

static u8* GLASS_getTexDataPtr(const glassTexture* tex, size_t level) {
    ASSERT(tex);
    const size_t offset = GLASS_utility_texOffset(tex->width, tex->height, tex->format, tex->dataType, level);
    return (((u8*)&tex->subTextures[tex->numOfSubTextures]) + offset);
}

static size_t GLASS_getTexDataAllocSize(const glassTexture* tex) {
    ASSERT(tex);
    return GLASS_utility_texAllocSize(tex->width, tex->height, tex->format, tex->dataType, tex->levels);
}

static GLenum GLASS_readTexHeader(TexStream* stream, GPU_TEXTURE_MODE_PARAM type, glassTexture* out) {
    ASSERT(stream);
    ASSERT(out);
    ASSERT((type == GPU_TEX_2D) || (type == GPU_TEX_CUBE_MAP));

    RawHeader header;
    stream->read(stream, &header, sizeof(RawHeader));
    if (header.type != type)
        return GL_INVALID_OPERATION;

    out->width = (1 << (header.widthLog2 + 3));
    out->height = (1 << (header.heightLog2 + 3));
    out->format = GLASS_utility_wrapTexFormat(header.format);
    out->dataType = GLASS_utility_getTexDataType(header.format);
    out->levels = (header.mipmapLevels + 1); // Add one for base level.

    if (type == GPU_TEX_2D) {
        out->numOfSubTextures = header.numSubTextures;
    } else {
        // It doesn't make sense for a cubemap to have sub-textures.
        ASSERT(header.numSubTextures == 0);
        out->numOfSubTextures = 0; 
    }

    return 0;
}

static GLenum GLASS_loadTextureImpl(TexStream* stream, glassTexture** out) {
    ASSERT(stream);
    ASSERT(out);
    
    // Parse header.
    glassTexture dummy;
    GLenum ret = GLASS_readTexHeader(stream, GPU_TEX_2D, &dummy);
    if (ret)
        return ret;

    // Allocate texture object.
    const size_t allocSize = GLASS_getTexDataAllocSize(&dummy);
    const size_t subTexSize = (sizeof(glassSubTexture) * dummy.numOfSubTextures);
    glassTexture* tex = glassVirtualAlloc(sizeof(glassTexture) + subTexSize + allocSize);
    if (!tex)
        return GL_OUT_OF_MEMORY;

    memcpy(tex, &dummy, sizeof(glassTexture));

    // Load sub-texture info.
    for (size_t i = 0; i < tex->numOfSubTextures; ++i) {
        RawSubTexture raw;
        stream->read(stream, &raw, sizeof(RawSubTexture));

        glassSubTexture* subTex = &tex->subTextures[i];
        subTex->xFactor = (raw.left / 1024.0f);
        subTex->yFactor = (1.0f - (raw.top / 1024.0f)); // TODO: make sure this is right.
        subTex->width = raw.width;
        subTex->height = raw.height;
    }

    // Load texture data.
    u8* data = GLASS_getTexDataPtr(tex, 0);
    ASSERT(decompress(data, allocSize, stream->read, (void*)stream, 0));

    *out = tex;
    return 0;
}

static GLenum GLASS_loadCubeMapImpl(TexStream* stream, glassTexture** outs) {
    ASSERT(stream);
    ASSERT(outs);

    // Parse header.
    glassTexture dummy;
    GLenum ret = GLASS_readTexHeader(stream, GPU_TEX_CUBE_MAP, &dummy);
    if (ret)
        return ret;

    // Allocate texture objects.
    const size_t allocSize = GLASS_getTexDataAllocSize(&dummy);
    glassTexture* texs[6];

    for (size_t i = 0; i < 6; ++i) {
        texs[i] = glassVirtualAlloc(sizeof(glassTexture) + allocSize);
        if (!texs[i]) {
            for (size_t j = 0; j < i; ++j)
                glassVirtualFree(texs[j]);

            return GL_OUT_OF_MEMORY;
        }

        memcpy(&texs[i], &dummy, sizeof(glassTexture));
    }

    // Load texture data.
    decompressIOVec iov[6];
    
    for (size_t i = 0; i < 6; ++i) {
        iov[i].data = GLASS_getTexDataPtr(texs[i], 0);
        iov[i].size = allocSize;
    }

    ASSERT(decompressV(iov, 6, stream->read, (void*)stream, 0));

    for (size_t i = 0; i < 6; ++i)
        outs[i] = texs[i];

    return 0;
}

static ssize_t GLASS_texStreamReadMem(void* userdata, void* out, size_t size) {
    TexStream* stream = (TexStream*)userdata;
    ASSERT(stream);
    const size_t actualSize = MIN(size, stream->size- stream->offset);
    memcpy(out, (u8*)(stream->handle) + stream->offset, actualSize);
    stream->offset += actualSize;
    return actualSize;
}

static ssize_t GLASS_texStreamReadFile(void* userdata, void* out, size_t size) {
    TexStream* stream = (TexStream*)userdata;
    ASSERT(stream);
    const size_t actualSize = fread(out, 1, size, (FILE*)stream->handle);
    stream->offset += actualSize;
    return actualSize;
}

static void GLASS_getMemTexStream(TexStream* stream, const u8* data, size_t size) {
    ASSERT(stream);
    stream->handle = (void*)data;
    stream->offset = 0;
    stream->size = size;
    stream->read = GLASS_texStreamReadMem;
}

static void GLASS_getFileTexStream(TexStream* stream, FILE* f) {
    ASSERT(stream);
    ASSERT(fseek(f, 0, SEEK_END) == 0);
    stream->handle = (void*)f;
    stream->offset = 0;
    stream->size = ftell(f);
    stream->read = GLASS_texStreamReadFile;
    ASSERT(fseek(f, 0, SEEK_SET) == 0);
}

GLenum glassLoadTexture(const u8* data, size_t size, glassTexture** out) {
    if (!data || !out)
        return GL_INVALID_VALUE;

    TexStream stream;
    GLASS_getMemTexStream(&stream, data, size);
    return GLASS_loadTextureImpl(&stream, out);
}

GLenum glassLoadCubeMap(const u8* data, size_t size, glassTexture** out) {
    if (!data || !out)
        return GL_INVALID_VALUE;

    TexStream stream;
    GLASS_getMemTexStream(&stream, data, size);
    return GLASS_loadCubeMapImpl(&stream, out);
}

GLenum glassLoadTextureFromFile(FILE* f, glassTexture** out) {
    if ((f == NULL) || !out)
        return GL_INVALID_VALUE;

    TexStream stream;
    GLASS_getFileTexStream(&stream, f);
    return GLASS_loadTextureImpl(&stream, out);
}

GLenum glassLoadCubeMapFromFile(FILE* f, glassTexture** out) {
    if ((f == NULL) || !out)
        return GL_INVALID_VALUE;

    TexStream stream;
    GLASS_getFileTexStream(&stream, f);
    return GLASS_loadCubeMapImpl(&stream, out);
}

GLenum glassLoadTextureFromPath(const char* path, glassTexture** out) {
    if (!path || !out)
        return GL_INVALID_VALUE;

    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return GL_INVALID_OPERATION;

    GLenum ret = glassLoadTextureFromFile(f, out);
    fclose(f);
    return ret;
}

GLenum glassLoadCubeMapFromPath(const char* path, glassTexture** out) {
    if (!path || !out)
        return GL_INVALID_VALUE;

    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return GL_INVALID_OPERATION;

    GLenum ret = glassLoadCubeMapFromFile(f, out);
    fclose(f);
    return ret;
}

void glassDestroyTexture(glassTexture* tex) { glassVirtualFree(tex); }

const size_t glassGetTextureSize(const glassTexture* tex, size_t level) {
    if (tex && (level < tex->levels))
        return GLASS_utility_texSize(tex->width, tex->height, tex->format, tex->dataType, level);

    return 0;
}

const u8* glassGetTextureData(const glassTexture* tex, size_t level) {
    if (tex && (level < tex->levels))
        return GLASS_getTexDataPtr(tex, level);

    return NULL;
}

const u8* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level) {
    // TODO
    return NULL;
}

bool glassIsTextureCompressed(const glassTexture* tex) {
    if (tex)
        return ((tex->format == GL_ETC1_RGB8_OES) || (tex->format == GL_ETC1_ALPHA_RGB8_A4_PICA));

    return false;
}