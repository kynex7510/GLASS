#include "Context.h"

void glActiveTexture(GLenum texture);

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
    switch (target) {
        case GL_TEXTURE_2D:
            ctx->texture2d = texture;
            break;
        case GL_TEXTURE_CUBE_MAP:
            ctx->textureCubeMap = texture;
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

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    ASSERT(textures);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = textures[i];

        // Validate name.
        if (!OBJ_IS_TEXTURE(name))
            continue;

        TextureInfo* info = (TextureInfo*)name;

        // TODO: unbind if bound.

        // Delete buffer.
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

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);

GLboolean glIsTexture(GLuint texture) {
    if (OBJ_IS_TEXTURE(texture)) {
        const TextureInfo* info = (TextureInfo*)texture;
        return info->flags & TEXTURE_FLAG_BOUND;
    }

    return GL_FALSE;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);