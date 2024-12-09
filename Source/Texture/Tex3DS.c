#include "Context.h"
#include "Utility.h"
#include "GX.h"

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

static u8* GLASS_getTexDataPtr(const glassTexture* tex, size_t face, size_t level) {
    u8* p = tex->faces[face];
    if (p) {
        const size_t offset = GLASS_utility_texOffset(tex->width, tex->height, tex->format, tex->type, level);
        return p + offset;
    }

    return NULL;
}

static size_t GLASS_getTexFaceAllocSize(const glassTexture* tex) {
    ASSERT(tex);
    return GLASS_utility_texAllocSize(tex->width, tex->height, tex->format, tex->type, tex->levels);
}

static void GLASS_loadTextureImpl(TexStream* stream, glassTexture* out) {
    ASSERT(stream);
    ASSERT(out);
    
    // Parse header.
    RawHeader header;
    stream->read(stream, &header, sizeof(RawHeader));
    ASSERT((header.type == GPU_TEX_2D) || (header.type == GPU_TEX_CUBE_MAP));

    for (size_t i = 0; i < 6; ++i)
        out->faces[i] = NULL;

    out->isCubeMap = header.type == GPU_TEX_CUBE_MAP;
    out->width = (1 << (header.widthLog2 + 3));
    out->height = (1 << (header.heightLog2 + 3));
    out->format = GLASS_utility_wrapTexFormat(header.format);
    out->type = GLASS_utility_getTexDataType(header.format);
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
    const size_t faceAllocSize = GLASS_getTexFaceAllocSize(out);
    const size_t numFaces = out->isCubeMap ? 6 : 1;
    for (size_t i = 0; i < numFaces; ++i) {
        out->faces[i] = glassLinearAlloc(faceAllocSize);
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
    decompressIOVec iov[6];

    for (size_t i = 0; i < numFaces; ++i) {
        u8* p = GLASS_getTexDataPtr(out, i, 0);
        ASSERT(p);
        iov[i].data = p;
        iov[i].size = faceAllocSize;
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

    const size_t faceAllocSize = GLASS_getTexFaceAllocSize(tex);
    const size_t numBuffers = (tex->isCubeMap ? 6 : 1);

    u8* oldBuffers[6];
    memcpy(oldBuffers, dest->data, 6 * sizeof(u8*));

    bool failed = false;

    for (size_t i = 0; i < 6; ++i) {
        if ((i < numBuffers) && (dest->flags & TEXTURE_FLAG_VRAM)) {
            dest->data[i] = glassVRAMAlloc(faceAllocSize, VRAM_ALLOC_ANY);
            if (!dest->data[i]) {
                GLASS_context_setError(GL_OUT_OF_MEMORY);
                failed = true;
                break;
            }

            ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache(tex->faces[i], faceAllocSize)));
            GLASS_gx_copyTexture((u32)tex->faces[i], (u32)dest->data[i], faceAllocSize);
        } else {
            dest->data[i] = tex->faces[i];
        }
    }

    if (failed) {
        for (size_t i = 0; i < 6; ++i) {
            if (dest->flags & TEXTURE_FLAG_VRAM)
                glassVRAMFree(dest->data[i]);

            dest->data[i] = oldBuffers[i];
        }
    } else {
        for (size_t i = 0; i < 6; ++i) {
            glassLinearFree(oldBuffers[i]);
            if (dest->flags & TEXTURE_FLAG_VRAM)
                glassLinearFree(tex->faces[i]);

            tex->faces[i] = NULL;
        }

        dest->width = tex->width;
        dest->height = tex->height;
        dest->format = tex->format;
        dest->dataType = tex->type;
        dest->flags |= TEXTURE_FLAG_INITIALIZED;
        ctx->flags |= CONTEXT_FLAG_TEXTURE;
    }
}

void glassDestroyTexture(glassTexture* tex) {
    if (!tex)
        return;

    for (size_t i = 0; i < (tex->isCubeMap ? 6 : 1); ++i)
        glassLinearFree(tex->faces[i]);

    glassVirtualFree(tex->subTextures);
}

const size_t glassGetTextureSize(const glassTexture* tex, size_t level) {
    if (tex && (level < tex->levels))
        return GLASS_utility_texSize(tex->width, tex->height, tex->format, tex->type, level);

    return 0;
}

const u8* glassGetTextureData(const glassTexture* tex, size_t face, size_t level) {
    if (tex && (level < tex->levels))
        return GLASS_getTexDataPtr(tex, face, level);

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