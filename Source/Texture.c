#include "Context.h"

#define IS_FILTER(filter) \
    (((filter) == GL_NEAREST) || ((filter) == GL_LINEAR))

#define IS_WRAP(wrap) \
    (((wrap) == GL_CLAMP_TO_EDGE) || ((wrap) == GL_CLAMP_TO_BORDER) || ((wrap) == GL_MIRRORED_REPEAT) || ((wrap) == GL_REPEAT))

void glBindTexture(GLenum target, GLuint texture) {
    ASSERT(OBJ_IS_TEXTURE(texture) || texture == GLASS_INVALID_OBJECT);

    // Check for previous binding.
    TextureInfo* info = (TextureInfo*)texture;
    if (info && (info->flags & TEXTURE_FLAG_BOUND) && (info->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Bind texture to context.
    CtxCommon* ctx = GLASS_context_getCommon();
    TextureUnit* unit = &ctx->textureUnits[ctx->activeTextureUnit];

    switch (target) {
        case GL_TEXTURE_2D:
            unit->texture2d = texture;
            break;
        case GL_TEXTURE_CUBE_MAP:
            unit->textureCubeMap = texture;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
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

            if (unit->texture2d == name)
                unit->texture2d = GLASS_INVALID_OBJECT;

            if (unit->textureCubeMap == name)
                unit->textureCubeMap = GLASS_INVALID_OBJECT;
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
    if (texture < GL_TEXTURE0 || texture > GL_TEXTURE3) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    ctx->activeTextureUnit = (texture - GL_TEXTURE0);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    const bool invalidFilter = ((pname == GL_TEXTURE_MIN_FILTER) || (pname == GL_TEXTURE_MAG_FILTER)) && !IS_FILTER(param);
    const bool invalidWrap = ((pname == GL_TEXTURE_WRAP_S) || (pname == GL_TEXTURE_WRAP_T)) && !IS_WRAP(param);
    if (invalidFilter || invalidWrap) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    // Get texture.
    CtxCommon* ctx = GLASS_context_getCommon();
    const TextureUnit* unit = &ctx->textureUnits[ctx->activeTextureUnit];
    TextureInfo* info = NULL;

    switch (target) {
        case GL_TEXTURE_2D:
            info = (TextureInfo*)unit->texture2d;
            break;
        case GL_TEXTURE_CUBE_MAP:
            info = (TextureInfo*)unit->textureCubeMap;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    // We don't support default textures.
    if (!info) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Set parameter.
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            info->minFilter = param;
            break;
        case GL_TEXTURE_MAG_FILTER:
            info->maxFilter = param;
            break;
        case GL_TEXTURE_WRAP_S:
            info->wrapS = param;
            break;
        case GL_TEXTURE_WRAP_T:
            info->wrapT = param;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) { glTexParameteri(target, pname, (GLint)param); }

void glGetTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    ASSERT(params);
    glTexParameterf(target, pname, params[0]);
}

void glGetTexParameteriv(GLenum target, GLenum pname, const GLint* params) {
    ASSERT(params);
    glTexParameteri(target, pname, params[0]);
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);