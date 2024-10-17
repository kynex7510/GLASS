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

static size_t GLASS_getTexDataOffset(const glassTexture* tex, size_t level) {
    ASSERT(tex);
    ASSERT(level <= 10);

    if (level > 0) {
        // Basically rewrite the offset in terms of the previous level dimensions:
        // B * W(L-1) * H(L-1) * (2^2(L-1) + 2^2(L-2) + ... + 2^2(L-L)) / 8
        const u16 prevWidth = (tex->width >> (level - 1));
        const u16 prevHeight = (tex->height >> (level - 1));
        const size_t bpp = GLASS_utility_getTexBitsPerPixel(GLASS_utility_getTexFormat(tex->format, tex->dataType));
        return ((bpp * prevWidth * prevHeight * (((1u << (level << 1)) - 1) & 0x55555)) >> 3);
    }

    return 0;
}

static INLINE size_t GLASS_getTexDataAllocSize(const glassTexture* tex) {
    ASSERT(tex);
    return GLASS_getTexDataOffset(tex, tex->levels);
}

static INLINE u8* GLASS_getTexDataPtr(const glassTexture* tex, size_t level) {
    ASSERT(tex);
    return ((u8*)&tex->subTextures[tex->numOfSubTextures]) + GLASS_getTexDataOffset(tex, level);
}

static GLenum GLASS_loadTextureImpl(TexStream* stream, glassTexture** out) {
    ASSERT(stream);
    ASSERT(out);

    // Read texture header.
    RawHeader header;
    stream->read(stream, &header, sizeof(RawHeader));
    if (header.type != GPU_TEX_2D)
        return GL_INVALID_OPERATION;

    // Create texture object.
    glassTexture dummy;
    dummy.width = (1 << (header.widthLog2 + 3));
    dummy.height = (1 << (header.heightLog2 + 3));
    dummy.format = GLASS_utility_wrapTexFormat(header.format);
    dummy.dataType = GLASS_utility_getTexDataType(header.format);
    dummy.levels = header.mipmapLevels;
    dummy.numOfSubTextures = header.numSubTextures;

    const size_t dataSize = GLASS_getTexDataAllocSize(&dummy);
    const size_t subTexSize = (sizeof(glassSubTexture) * dummy.numOfSubTextures);
    glassTexture* tex = glassVirtualAlloc(sizeof(glassTexture) + subTexSize + dataSize);
    if (!tex)
        return GL_OUT_OF_MEMORY;

    memcpy(tex, &dummy, sizeof(glassTexture));

    for (size_t i = 0; i < tex->numOfSubTextures; ++i) {
        RawSubTexture raw;
        stream->read(stream, &raw, sizeof(RawSubTexture));

        glassSubTexture* subTex = &tex->subTextures[i];
        subTex->width = raw.width;
        subTex->height = raw.height;
        subTex->top = raw.top / 1024.0f;
        subTex->bottom = raw.bottom / 1024.0f;
        subTex->left = raw.left / 1024.0f;
        subTex->right = raw.right / 1024.0f;
    }

    // Load texture data.
    u8* data = GLASS_getTexDataPtr(tex, 0);
	ASSERT(decompress(data, dataSize, stream->read, (void*)stream, 0));

    *out = tex;
    return 0;
}

static GLenum GLASS_loadCubeMapImpl(TexStream* stream, glassTexture** outs) {
    ASSERT(stream);
    ASSERT(outs);

    // Read texture header.
    RawHeader header;
    stream->read(stream, &header, sizeof(RawHeader));
    if (header.type != GPU_TEX_CUBE_MAP)
        return GL_INVALID_OPERATION;

    // Create texture objects.
    glassTexture dummy;
    dummy.width = (1 << (header.widthLog2 + 3));
    dummy.height = (1 << (header.heightLog2 + 3));
    dummy.format = GLASS_utility_wrapTexFormat(header.format);
    dummy.dataType = GLASS_utility_getTexDataType(header.format);
    dummy.levels = header.mipmapLevels;

    // TODO: does it make sense for a cubemap to have subtextures?
    ASSERT(header.numSubTextures == 0);
    dummy.numOfSubTextures = 0; 

    const size_t dataSize = GLASS_getTexDataAllocSize(&dummy);
    glassTexture* texs[6];

    for (size_t i = 0; i < 6; ++i) {
        texs[i] = glassVirtualAlloc(sizeof(glassTexture) + dataSize);
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
		iov[i].size = dataSize;
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

static INLINE void GLASS_getMemTexStream(TexStream* stream, const u8* data, size_t size) {
    ASSERT(stream);
    stream->handle = (void*)data;
    stream->offset = 0;
    stream->size = size;
    stream->read = GLASS_texStreamReadMem;
}

static INLINE void GLASS_getFileTexStream(TexStream* stream, FILE* f) {
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

void glassDestroyTexture(glassTexture* tex) {
    if (tex)
        glassVirtualFree(tex);
}

const size_t glassGetTextureSize(const glassTexture* tex, size_t level) {
    if (tex && (level < (tex->levels - 1))) {
        const u16 width = (tex->width >> level);
        const u16 height = (tex->height >> level);
        const size_t bpp = GLASS_utility_getTexBitsPerPixel(GLASS_utility_getTexFormat(tex->format, tex->dataType));
        return ((width * height * bpp) >> 3);
    }

    return 0;
}

const u8* glassGetTextureData(const glassTexture* tex, size_t level) {
    if (tex && (level < (tex->levels - 1)))
        return GLASS_getTexDataPtr(tex, level);

    return NULL;
}

const u8* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level); // TODO

bool glassIsTextureCompressed(const glassTexture* tex) {
    if (tex)
        return ((tex->format == GL_ETC1_RGB8_OES) || (tex->format == GL_ETC1_ALPHA_RGB8_A4_PICA));

    return false;
}