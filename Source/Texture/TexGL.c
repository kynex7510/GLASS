#include "Base/Context.h"
#include "Texture/Texture.h"

#include <string.h>

static bool GLASS_isMagFilter(GLenum filter) {
    switch (filter) {
        case GL_NEAREST:
        case GL_LINEAR:
            return true;
    }

    return false;
}

static bool GLASS_isMinFilter(GLenum filter) {
    if (GLASS_isMagFilter(filter))
        return true;

    switch (filter) {
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_LINEAR:
            return true;
    }

    return false;
}

static bool GLASS_isTexWrap(GLenum wrap) {
    switch (wrap) {
        case GL_CLAMP_TO_EDGE:
        case GL_CLAMP_TO_BORDER:
        case GL_MIRRORED_REPEAT:
        case GL_REPEAT:
            return true;
    }

    return false;
}

static bool GLASS_isTexFormat(GLenum format) {
    switch (format) {
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
        case GL_RGB:
        case GL_RGBA:
            return true;
    }

    return false;
}

static bool GLASS_isTexType(GLenum type) {
    switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_NIBBLE_PICA:
        case GL_UNSIGNED_BYTE_4_4_PICA:
            return true;
    }

    return false;
}

static bool GLASS_isTexCompressedFormat(GLenum format) {
    switch (format) {
        case GL_ETC1_RGB8_OES:
        case GL_ETC1_ALPHA_RGB8_A4_PICA:
            return true;
    }

    return false;
}

static GLenum GLASS_texTargetForSubtarget(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return GL_TEXTURE_2D;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return GL_TEXTURE_CUBE_MAP;
    }

    UNREACHABLE("Invalid texture subtarget!");
}

void glBindTexture(GLenum target, GLuint texture) {
    ASSERT(OBJ_IS_TEXTURE(texture) || texture == GLASS_INVALID_OBJECT);

    if ((target != GL_TEXTURE_2D) && (target != GL_TEXTURE_CUBE_MAP)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    // Check for previous binding.
    TextureInfo* tex = (TextureInfo*)texture;
    if (tex && (tex->flags & TEXTURE_FLAG_BOUND) && (tex->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Only texture0 supports cube maps.
    CtxCommon* ctx = GLASS_context_getCommon();
    const bool hasCubeMap = (target == GL_TEXTURE_CUBE_MAP);

    if (hasCubeMap && ctx->activeTextureUnit) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Bind texture to context.
    if (texture != ctx->textureUnits[ctx->activeTextureUnit]) {
        ctx->textureUnits[ctx->activeTextureUnit] = texture;
        ctx->flags |= CONTEXT_FLAG_TEXTURE;
    }

    if (tex) {
        tex->target = target;
        tex->flags |= TEXTURE_FLAG_BOUND;
    }
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    ASSERT(textures);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    for (size_t i = 0; i < n; ++i) {
        GLuint name = textures[i];

        // Validate name.
        if (!OBJ_IS_TEXTURE(name))
            continue;

        TextureInfo* tex = (TextureInfo*)name;

        // Unbind if bound.
        for (size_t i = 0; i < GLASS_NUM_TEX_UNITS; ++i) {
            if (ctx->textureUnits[i] == name) {
                ctx->textureUnits[i] = GLASS_INVALID_OBJECT;
                ctx->flags |= CONTEXT_FLAG_TEXTURE;
            }
        }

        // Delete texture.
        const bool useVRAM = tex->flags |= TEXTURE_FLAG_VRAM;
        for (size_t j = 0; j < GLASS_NUM_TEX_FACES; ++j) {
            u8* p = tex->faces[i];
            useVRAM ? glassVRAMFree(p) : glassLinearFree(p);
        }

        glassVirtualFree(tex);
    }
}

void glGenTextures(GLsizei n, GLuint* textures) {
    ASSERT(textures);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_TEXTURE_TYPE);
        if (!OBJ_IS_TEXTURE(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        TextureInfo* tex = (TextureInfo*)name;
        tex->target = 0;
        tex->format = 0;
        tex->dataType = 0;
        tex->borderColor = 0;
        tex->width = 0;
        tex->height = 0;
        tex->minFilter = GL_NEAREST_MIPMAP_LINEAR;
        tex->magFilter = GL_LINEAR;
        tex->wrapS = GL_REPEAT;
        tex->wrapT = GL_REPEAT;
        tex->minLod = 0;
        tex->maxLod = 0;
        tex->flags = 0;
        tex->lodBias = 0;

        for (size_t j = 0; j < GLASS_NUM_TEX_FACES; ++j)
            tex->faces[j] = NULL;

        textures[i] = name;
    }
}

GLboolean glIsTexture(GLuint texture) {
    if (OBJ_IS_TEXTURE(texture)) {
        const TextureInfo* tex = (TextureInfo*)texture;
        return tex->flags & TEXTURE_FLAG_BOUND;
    }

    return GL_FALSE;
}

void glActiveTexture(GLenum texture) {
    if ((texture < GL_TEXTURE0) || (texture > GL_TEXTURE2)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->activeTextureUnit = (texture - GL_TEXTURE0);
}

static bool GLASS_setTexInt(TextureInfo* tex, GLenum pname, GLint param) {
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            tex->minFilter = param;
            return true;
        case GL_TEXTURE_MAG_FILTER:
            tex->magFilter = param;
            return true;
        case GL_TEXTURE_WRAP_S:
            tex->wrapS = param;
            return true;
        case GL_TEXTURE_WRAP_T:
            tex->wrapT = param;
            return true;
        case GL_TEXTURE_MIN_LOD:
            tex->minLod = param;
            return true;
        case GL_TEXTURE_MAX_LOD:
            tex->maxLod = param;
            return true;
    }

    return false;
}

static bool GLASS_setTexFloats(TextureInfo* tex, GLenum pname, const GLfloat* params) {
    ASSERT(params);

    switch (pname) {
        case GL_TEXTURE_BORDER_COLOR:
            tex->borderColor = (((u32)(params[3] * 0xFF) << 24) | ((u32)(params[2] * 0xFF) << 16) | ((u32)(params[1] * 0xFF) << 8) | (u32)(params[0] * 0xFF));
            return true;
        case GL_TEXTURE_LOD_BIAS:
            tex->lodBias = params[0];
            return true;
    }

    return false;
}

static bool GLASS_validateTexParam(GLenum pname, GLenum param) {
    const bool invalidMinFilter = ((pname == GL_TEXTURE_MIN_FILTER) && !GLASS_isMinFilter(param));
    const bool invalidMagFilter = ((pname == GL_TEXTURE_MAG_FILTER) && !GLASS_isMagFilter(param));
    const bool invalidWrap = (((pname == GL_TEXTURE_WRAP_S) || (pname == GL_TEXTURE_WRAP_T)) && !GLASS_isTexWrap(param));
    return (!invalidMinFilter && !invalidMagFilter && !invalidWrap);
}

static void GLASS_setTexParams(GLenum target, GLenum pname, const GLint* intParams, const GLfloat* floatParams) {
    // Get texture.
    CtxCommon* ctx = GLASS_context_getCommon();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    // We don't support default textures, and only one target at time can be used.
    if (!tex || (tex->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Set parameters.
    if (intParams) {
        ASSERT(!floatParams);

        const GLenum param = intParams[0];
        if (!GLASS_validateTexParam(pname, param)) {
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
        }

        if (!GLASS_setTexInt(tex, pname, param)) {
            GLfloat floatParams[4];

            // Integer color components are interpreted linearly such that the most positive integer maps to 1.0
            // and the most negative integer maps to -1.0.
            if (pname == GL_TEXTURE_BORDER_COLOR) {
                for (size_t i = 0; i < 4; ++i) {
                    floatParams[i] = (GLfloat)(intParams[i]) / 0x7FFFFFFF;
                }
            } else {
                // Only other choice is lod bias.
                floatParams[0] = (GLfloat)intParams[0];
            }

            if (!GLASS_setTexFloats(tex, pname, floatParams))
                GLASS_context_setError(GL_INVALID_ENUM);
        }
    } else if (floatParams) {
        if (!GLASS_setTexFloats(tex, pname, floatParams)) {
            if (!GLASS_validateTexParam(pname, floatParams[0]) || !GLASS_setTexInt(tex, pname, floatParams[0]))
                GLASS_context_setError(GL_INVALID_ENUM);
        }
    }
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) { GLASS_setTexParams(target, pname, NULL, params); }
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) { GLASS_setTexParams(target, pname, params, NULL); }
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) { glTexParameterfv(target, pname, &param); }
void glTexParameteri(GLenum target, GLenum pname, GLint param) { glTexParameteriv(target, pname, &param); }

static bool GLASS_checkTexArgs(GLenum target, GLint level, GLsizei width, GLsizei height, GLint border) {
    if ((target != GL_TEXTURE_2D) && (width != height))
        return false;

    if ((width < GLASS_MIN_TEX_SIZE) || (height < GLASS_MIN_TEX_SIZE))
        return false;

    if ((width > GLASS_MAX_TEX_SIZE) || (height > GLASS_MAX_TEX_SIZE))
        return false;

    if ((level < 0) || (level >= GLASS_NUM_TEX_LEVELS) || (border != 0))
        return false;

    return true;
}

static void GLASS_setTex(GLenum target, GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, const u8* data) {
    CtxCommon* ctx = GLASS_context_getCommon();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    // We don't support default textures.
    if (!tex) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Target check.
    if (tex->target != GLASS_texTargetForSubtarget(target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Only texture0 supports cube maps.
    const bool hasCubeMap = (target != GL_TEXTURE_2D);
    if (hasCubeMap && ctx->activeTextureUnit) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }
    
    size_t face;
    switch (target) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            face = 0;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            face = 1;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            face = 2;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            face = 3;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            face = 4;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            face = 5;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    // Prepare memory.
    TexReallocStatus reallocStatus = GLASS_tex_realloc(tex, width << level, height << level, format, type, tex->flags & TEXTURE_FLAG_VRAM);
    if (reallocStatus == TexReallocStatus_Failed)
        return;

    // Write data.
    if (data) {
        GLASS_tex_writeRaw(tex, data, face, level);
        reallocStatus = TexReallocStatus_Updated;
    }
    
    if (reallocStatus == TexReallocStatus_Updated)
        ctx->flags |= CONTEXT_FLAG_TEXTURE;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data) {
    if (!GLASS_checkTexArgs(target, level, width, height, border)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }
    
    if (!GLASS_isTexFormat(format) || !GLASS_isTexType(type)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }
    
    if ((format != internalformat) || !GLASS_tex_isFormatValid(format, type)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    GLASS_setTex(target, level, width, height, format, type, (const u8*)data);    
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {
    if (!GLASS_checkTexArgs(target, level, width, height, border)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }
    
    if (!GLASS_isTexCompressedFormat(internalformat)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    //GLASS_setTex(target, level, width, height, internalformat, 0, (const u8*)data);
}

void glTexVRAMPICA(GLboolean enabled) {
    CtxCommon* ctx = GLASS_context_getCommon();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    if (!tex) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    const bool hadVRAM = tex->flags & TEXTURE_FLAG_VRAM;
    if (hadVRAM == enabled)
        return;

    if (!(tex->flags & TEXTURE_FLAG_INITIALIZED)) {
        if (enabled) {
            tex->flags |= TEXTURE_FLAG_VRAM;
        } else {
            tex->flags &= ~(TEXTURE_FLAG_VRAM);
        }

        return;
    }

    const TexReallocStatus reallocStatus = GLASS_tex_realloc(tex, tex->width, tex->height, tex->format, tex->dataType, enabled);
    if (reallocStatus == TexReallocStatus_Updated)
        ctx->flags |= CONTEXT_FLAG_TEXTURE;
}

// TODO
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);