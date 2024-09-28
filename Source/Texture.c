#include "Context.h"

#include <stdint.h> // INT32_MAX

#define IS_MAG_FILTER(filter) \
    (((filter) == GL_NEAREST) || ((filter) == GL_LINEAR))

#define IS_MIN_FILTER(filter) \
    ((IS_MAG_FILTER((filter))) || ((filter) == GL_NEAREST_MIPMAP_NEAREST) || ((filter) == GL_NEAREST_MIPMAP_LINEAR) || ((filter) == GL_LINEAR_MIPMAP_NEAREST) || ((filter) == GL_LINEAR_MIPMAP_LINEAR))

#define IS_WRAP(wrap) \
    (((wrap) == GL_CLAMP_TO_EDGE) || ((wrap) == GL_CLAMP_TO_BORDER) || ((wrap) == GL_MIRRORED_REPEAT) || ((wrap) == GL_REPEAT))

void glBindTexture(GLenum target, GLuint texture) {
    ASSERT(OBJ_IS_TEXTURE(texture) || texture == GLASS_INVALID_OBJECT);

    if ((target != GL_TEXTURE_2D) && (target != GL_TEXTURE_CUBE_MAP)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    // Check for previous binding.
    TextureInfo* info = (TextureInfo*)texture;
    if (info && (info->flags & TEXTURE_FLAG_BOUND) && (info->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Only texture0 supports cube maps.
    CtxCommon* ctx = GLASS_context_getCommon();
    TextureUnit* unit = &ctx->textureUnits[ctx->activeTextureUnit];
    const bool hasCubeMap = (target == GL_TEXTURE_CUBE_MAP);

    if (hasCubeMap && ctx->activeTextureUnit) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Bind texture to context.
    if (unit->texture != texture) {
        unit->texture = texture;
        unit->dirty = true;
        ctx->flags |= CONTEXT_FLAG_TEXTURE;
    }

    if (info) {
        info->target = target;
        info->flags |= TEXTURE_FLAG_BOUND;
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

        TextureInfo* info = (TextureInfo*)name;

        // Unbind if bound.
        for (size_t i = 0; i < GLASS_NUM_TEXTURE_UNITS; ++i) {
            TextureUnit* unit = &ctx->textureUnits[i];
            if (unit->texture == name) {
                unit->texture = GLASS_INVALID_OBJECT;
                unit->dirty = true;
                ctx->flags |= CONTEXT_FLAG_TEXTURE;
            }
        }

        // Delete texture.
        glassVirtualFree(info);
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

        TextureInfo* info = (TextureInfo*)name;
        info->flags = 0;
        textures[i] = name;
    }
}

GLboolean glIsTexture(GLuint texture) {
    if (OBJ_IS_TEXTURE(texture)) {
        const TextureInfo* info = (TextureInfo*)texture;
        return info->flags & TEXTURE_FLAG_BOUND;
    }

    return GL_FALSE;
}

void glActiveTexture(GLenum texture) {
    if (texture < GL_TEXTURE0 || texture > GL_TEXTURE2) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->activeTextureUnit = (texture - GL_TEXTURE0);
}

static bool GLASS_setTexInt(TextureInfo* info, GLenum pname, GLint param) {
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            info->minFilter = param;
            return true;
        case GL_TEXTURE_MAG_FILTER:
            info->magFilter = param;
            return true;
        case GL_TEXTURE_WRAP_S:
            info->wrapS = param;
            return true;
        case GL_TEXTURE_WRAP_T:
            info->wrapT = param;
            return true;
        case GL_TEXTURE_MIN_LOD:
            info->minLod = param > 0 ? param : 0;
            return true;
        case GL_TEXTURE_MAX_LOD:
            info->maxLod = param > 0 ? param : 0;
            return true;
    }

    return false;
}

static bool GLASS_setTexFloats(TextureInfo* info, GLenum pname, const GLfloat* params) {
    ASSERT(params);

    switch (pname) {
        case GL_TEXTURE_BORDER_COLOR:
            info->borderColor = (((u32)(params[3] * 0xFF) << 24) | ((u32)(params[2] * 0xFF) << 16) | ((u32)(params[1] * 0xFF) << 8) | (u32)(params[0] * 0xFF));
            return true;
        case GL_TEXTURE_LOD_BIAS:
            info->lodBias = params[0];
            return true;
    }

    return false;
}

static INLINE bool GLASS_validateTexParam(GLenum pname, GLenum param) {
    const bool invalidMinFilter = ((pname == GL_TEXTURE_MIN_FILTER) && !IS_MIN_FILTER(param));
    const bool invalidMagFilter = ((pname == GL_TEXTURE_MAG_FILTER) && !IS_MAG_FILTER(param));
    const bool invalidWrap = (((pname == GL_TEXTURE_WRAP_S) || (pname == GL_TEXTURE_WRAP_T)) && !IS_WRAP(param));
    return !invalidMinFilter && !invalidMagFilter && !invalidWrap;
}

static void GLASS_setTexParams(GLenum target, GLenum pname, const GLint* intParams, const GLfloat* floatParams) {
    // Get texture.
    CtxCommon* ctx = GLASS_context_getCommon();
    const TextureUnit* unit = &ctx->textureUnits[ctx->activeTextureUnit];
    TextureInfo* info = (TextureInfo*)unit->texture;

    // We don't support default textures, and only one target at time can be used.
    if (!info || info->target != target) {
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

        if (!GLASS_setTexInt(info, pname, param)) {
            GLfloat floatParams[4];

            // Integer color components are interpreted linearly such that the most positive integer maps to 1.0
            // and the most negative integer maps to -1.0.
            if (pname == GL_TEXTURE_BORDER_COLOR) {
                for (size_t i = 0; i < 4; ++i) {
                    floatParams[i] = (GLfloat)(intParams[i]) / INT32_MAX;
                }
            } else {
                // Only other choice is lod bias.
                floatParams[0] = (GLfloat)intParams[0];
            }

            if (!GLASS_setTexFloats(info, pname, floatParams))
                GLASS_context_setError(GL_INVALID_ENUM);
        }

    }
    else if (floatParams) {
        if (!GLASS_setTexFloats(info, pname, floatParams)) {
            if (!GLASS_validateTexParam(pname, floatParams[0]) || !GLASS_setTexInt(info, pname, floatParams[0]))
                GLASS_context_setError(GL_INVALID_ENUM);
        }
    }
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) { GLASS_setTexParams(target, pname, NULL, params); }
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) { GLASS_setTexParams(target, pname, params, NULL); }
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) { glTexParameterfv(target, pname, &param); }
void glTexParameteri(GLenum target, GLenum pname, GLint param) { glTexParameteriv(target, pname, &param); }

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);