#include "Base/Context.h"
#include "Base/Utility.h"
#include "Base/Texture.h"
#include "Base/Pixels.h"

#include <string.h> // memcpy, memset

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

static u8* GLASS_getTexDataPtr(const glassTexture* tex, size_t face, size_t level) {
    ASSERT(tex);

    u8* p = tex->faces[face];
    if (p) {
        const size_t offset = GLASS_tex_getOffset(tex->width, tex->height, &tex->pixelFormat, level);
        return p + offset;
    }

    return NULL;
}

static size_t GLASS_getTexDataSize(const glassTexture* tex) {
    ASSERT(tex);
    return GLASS_tex_getAllocSize(tex->width, tex->height, &tex->pixelFormat, tex->levels);
}

static size_t GLASS_getTexAllocSize(const glassTexture* tex) {
    ASSERT(tex);
    return GLASS_tex_getAllocSize(tex->width, tex->height, &tex->pixelFormat, -1);
}

static size_t GLASS_getNumFaces(bool isCubeMap) {
    return GLASS_tex_getNumFaces(isCubeMap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
}

static GLenum GLASS_wrapTexFormat(GPU_TEXCOLOR format) {
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
        case GPU_ETC1A4:
            return GL_ETC1_ALPHA_RGB8_A4_PICA;
    }

    UNREACHABLE("Invalid parameter!");
}

GLenum GLASS_wrapTexType(GPU_TEXCOLOR format) {
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
        // Compressed texture don't have a data type.
        case GPU_ETC1:
        case GPU_ETC1A4:
            return 0;
    }

    UNREACHABLE("Invalid parameter!");
}

static void GLASS_loadTextureImpl(TexStream* stream, glassTexture* out) {
    ASSERT(stream);
    ASSERT(out);
    
    // Parse header.
    RawHeader header;
    stream->read(stream, &header, sizeof(RawHeader));
    ASSERT((header.type == GPU_TEX_2D) || (header.type == GPU_TEX_CUBE_MAP));

    memset(out->faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));
    out->isCubeMap = header.type == GPU_TEX_CUBE_MAP;
    out->width = (1 << (header.widthLog2 + 3));
    ASSERT(out->width >= GLASS_MIN_TEX_SIZE);
    out->height = (1 << (header.heightLog2 + 3));
    ASSERT(out->height >= GLASS_MIN_TEX_SIZE);
    out->pixelFormat.format = GLASS_wrapTexFormat(header.format);
    out->pixelFormat.type = GLASS_wrapTexType(header.format);
    out->levels = (header.mipmapLevels + 1); // Add one for base level.

    if (!out->isCubeMap) {
        out->numOfSubTextures = header.numSubTextures;
    } else {
        // It doesn't make sense for a cubemap to have sub-textures.
        ASSERT(header.numSubTextures == 0);
        out->numOfSubTextures = 0; 
    }

    // Allocate sub textures.
    out->subTextures = glassVirtualAlloc(sizeof(glassSubTexture) * out->numOfSubTextures);
    if (!out->subTextures) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        return;
    }
    
    // Allocate texture data.
    const size_t dataSize = GLASS_getTexDataSize(out);
    const size_t allocSize = GLASS_getTexAllocSize(out);
    const size_t numFaces = GLASS_getNumFaces(out->isCubeMap);
    for (size_t i = 0; i < numFaces; ++i) {
        out->faces[i] = glassLinearAlloc(allocSize);
        if (!out->faces[i]) {
            glassDestroyTexture(out);
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }
    }

    // Load sub-texture info.
    for (size_t i = 0; i < out->numOfSubTextures; ++i) {
        RawSubTexture raw;
        stream->read(stream, &raw, sizeof(RawSubTexture));

        glassSubTexture* subTex = &out->subTextures[i];
        subTex->xFactor = (raw.left / 1024.0f);
        subTex->yFactor = (1.0f - (raw.top / 1024.0f)); // TODO: make sure this is right.
        subTex->width = raw.width;
        subTex->height = raw.height;
    }

    // Load texture data.
    decompressIOVec iov[GLASS_NUM_TEX_FACES];

    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = GLASS_getTexDataPtr(out, i, 0);
        ASSERT(p);
        iov[i].data = p;
        iov[i].size = dataSize;
    }

    ASSERT(decompressV(iov, numFaces, stream->read, (void*)stream, 0));
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

void glassLoadTexture(const u8* data, size_t size, glassTexture* out) {
    if (!data || !out) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    TexStream stream;
    GLASS_getMemTexStream(&stream, data, size);
    GLASS_loadTextureImpl(&stream, out);
}

void glassLoadTextureFromFile(FILE* f, glassTexture* out) {
    if ((f == NULL) || !out) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    TexStream stream;
    GLASS_getFileTexStream(&stream, f);
    GLASS_loadTextureImpl(&stream, out);
}

void glassLoadTextureFromPath(const char* path, glassTexture* out) {
    if (!path || !out) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    glassLoadTextureFromFile(f, out);
}

void glassMoveTextureData(glassTexture* tex) {
    if (!tex)
        return;

    CtxCommon* ctx = GLASS_context_getCommon();
    TextureInfo* dest = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    if (dest->target != (tex->isCubeMap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Check if texture has been moved already.
    if (!tex->faces[0]) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (dest->vram) {
        const TexReallocStatus reallocStatus = GLASS_tex_realloc(dest, tex->width, tex->height, &tex->pixelFormat, true);
        if (reallocStatus == TexReallocStatus_Failed)
            return;

        // Move data.
        const size_t allocSize = GLASS_getTexAllocSize(tex);
        const size_t numFaces = GLASS_getNumFaces(tex->isCubeMap);

        for (size_t i = 0; i < numFaces; ++i) {
            GLASS_tex_write(dest, tex->faces[i], allocSize, i, 0);
            glassLinearFree(tex->faces[i]);
        }
    } else {
        // Just move the pointers.
        GLASS_tex_set(dest, tex->width, tex->height, &tex->pixelFormat, false, tex->faces);
    }

    memset(tex->faces, 0, GLASS_NUM_TEX_FACES * sizeof(u8*));
    ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;
}

void glassDestroyTexture(glassTexture* tex) {
    if (!tex)
        return;

    const size_t numFaces = GLASS_getNumFaces(tex->isCubeMap);
    for (size_t i = 0; i < numFaces; ++i)
        glassLinearFree(tex->faces[i]);

    glassVirtualFree(tex->subTextures);
}

const size_t glassGetTextureSize(const glassTexture* tex, size_t level) {
    if (tex && (level < tex->levels))
        return (((tex->width >> level) * (tex->height >> level) * GLASS_pixels_bpp(&tex->pixelFormat)) >> 3);

    return 0;
}

const u8* glassGetTextureData(const glassTexture* tex, size_t face, size_t level) {
    if (tex && (level < tex->levels)) {
        if (face < GLASS_getNumFaces(tex->isCubeMap))
            return GLASS_getTexDataPtr(tex, face, level);
    }

    return NULL;
}

const u8* glassGetSubTextureData(const glassTexture* tex, const glassSubTexture* subTex, size_t level) {
    // TODO
    return NULL;
}

bool glassIsCompressed(const glassPixelFormat* pixelFormat) {
    if (pixelFormat)
        return ((pixelFormat->format == GL_ETC1_RGB8_OES) || (pixelFormat->format == GL_ETC1_ALPHA_RGB8_A4_PICA));

    return false;
}