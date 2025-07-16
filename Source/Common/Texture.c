/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Utility.h>

#include "Base/Context.h"
#include "Base/TexManager.h"

void glBindTexture(GLenum target, GLuint name) {
    KYGX_ASSERT(GLASS_OBJ_IS_TEXTURE(name) || name == GLASS_INVALID_OBJECT);

    if ((target != GL_TEXTURE_2D) && (target != GL_TEXTURE_CUBE_MAP)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    // Check for previous binding.
    TextureInfo* tex = (TextureInfo*)name;
    if (tex && (tex->target != GLASS_TEX_TARGET_UNBOUND) && (tex->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Only texture0 supports cube maps.
    CtxCommon* ctx = GLASS_context_getBound();
    const bool hasCubeMap = (target == GL_TEXTURE_CUBE_MAP);

    if (hasCubeMap && ctx->activeTextureUnit) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Bind texture to context.
    if (name != ctx->textureUnits[ctx->activeTextureUnit]) {
        ctx->textureUnits[ctx->activeTextureUnit] = name;
        ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;

        if (tex)
            tex->target = target;
    }
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    KYGX_ASSERT(textures);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();

    for (size_t i = 0; i < n; ++i) {
        GLuint name = textures[i];

        // Validate name.
        if (!GLASS_OBJ_IS_TEXTURE(name))
            continue;

        TextureInfo* tex = (TextureInfo*)name;

        // Unbind if bound.
        const size_t fbIndex = GLASS_context_getFBIndex(ctx);
        if (GLASS_OBJ_IS_FRAMEBUFFER(ctx->framebuffer[fbIndex])) {
            FramebufferInfo* fbInfo = (FramebufferInfo*)ctx->framebuffer[fbIndex];
            if (fbInfo->colorBuffer == name)
                fbInfo->colorBuffer = 0;
        }

        for (size_t i = 0; i < GLASS_NUM_TEX_UNITS; ++i) {
            if (ctx->textureUnits[i] == name) {
                ctx->textureUnits[i] = GLASS_INVALID_OBJECT;
                ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;
            }
        }

        // Delete texture.
        for (size_t j = 0; j < GLASS_NUM_TEX_FACES; ++j) {
            u8* p = tex->faces[i];
            tex->vram ? glassVRAMFree(p) : glassLinearFree(p);
        }

        glassHeapFree(tex);
    }
}

void glGenTextures(GLsizei n, GLuint* textures) {
    KYGX_ASSERT(textures);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_TEXTURE_TYPE);
        if (!GLASS_OBJ_IS_TEXTURE(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        TextureInfo* tex = (TextureInfo*)name;
        tex->target = GLASS_TEX_TARGET_UNBOUND;
        tex->format = TEXFORMAT_RGBA8;
        tex->minFilter = GL_NEAREST_MIPMAP_LINEAR;
        tex->magFilter = GL_LINEAR;
        tex->wrapS = GL_REPEAT;
        tex->wrapT = GL_REPEAT;
        textures[i] = name;
    }
}

GLboolean glIsTexture(GLuint texture) {
    if (GLASS_OBJ_IS_TEXTURE(texture)) {
        const TextureInfo* tex = (TextureInfo*)texture;
        if (tex->target != GLASS_TEX_TARGET_UNBOUND)
            return GL_TRUE;
    }

    return GL_FALSE;
}

void glActiveTexture(GLenum texture) {
    if ((texture < GL_TEXTURE0) || (texture > GL_TEXTURE2)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ctx->activeTextureUnit = (texture - GL_TEXTURE0);
}

static inline bool setTexInt(TextureInfo* tex, GLenum pname, GLint param) {
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

static inline bool setTexFloats(TextureInfo* tex, GLenum pname, const GLfloat* params) {
    KYGX_ASSERT(params);

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

static inline bool isMagFilter(GLenum filter) {
    switch (filter) {
        case GL_NEAREST:
        case GL_LINEAR:
            return true;
    }

    return false;
}

static inline bool isMinFilter(GLenum filter) {
    if (isMagFilter(filter))
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

static inline bool isTexWrap(GLenum wrap) {
    switch (wrap) {
        case GL_CLAMP_TO_EDGE:
        case GL_CLAMP_TO_BORDER:
        case GL_MIRRORED_REPEAT:
        case GL_REPEAT:
            return true;
    }

    return false;
}

static inline bool validateTexParam(GLenum pname, GLenum param) {
    const bool invalidMinFilter = ((pname == GL_TEXTURE_MIN_FILTER) && !isMinFilter(param));
    const bool invalidMagFilter = ((pname == GL_TEXTURE_MAG_FILTER) && !isMagFilter(param));
    const bool invalidWrap = (((pname == GL_TEXTURE_WRAP_S) || (pname == GL_TEXTURE_WRAP_T)) && !isTexWrap(param));
    return (!invalidMinFilter && !invalidMagFilter && !invalidWrap);
}

static inline void setTexParams(GLenum target, GLenum pname, const GLint* intParams, const GLfloat* floatParams) {
    // Get texture.
    CtxCommon* ctx = GLASS_context_getBound();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    // We don't support default textures, and only one target at time can be used.
    if (!tex || (tex->target != target)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Set parameters.
    if (intParams) {
        KYGX_ASSERT(!floatParams);

        const GLenum param = intParams[0];
        if (!validateTexParam(pname, param)) {
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
        }

        if (!setTexInt(tex, pname, param)) {
            GLfloat floatParams[4];

            // Integer color components are interpreted linearly such that the most positive integer maps to 1.0
            // and the most negative integer maps to -1.0.
            if (pname == GL_TEXTURE_BORDER_COLOR) {
                for (size_t i = 0; i < 4; ++i) {
                    floatParams[i] = (GLfloat)(intParams[i]) / 2147483648.0f;
                }
            } else {
                // Only other choice is lod bias.
                floatParams[0] = (GLfloat)intParams[0];
            }

            if (!setTexFloats(tex, pname, floatParams)) {
                GLASS_context_setError(GL_INVALID_ENUM);
                return;
            }
        }
    } else if (floatParams) {
        if (!setTexFloats(tex, pname, floatParams)) {
            if (!validateTexParam(pname, floatParams[0]) || !setTexInt(tex, pname, floatParams[0])) {
                GLASS_context_setError(GL_INVALID_ENUM);
                return;
            }
        }
    } else {
        KYGX_UNREACHABLE("Params expected!");
    }

    ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) { setTexParams(target, pname, NULL, params); }
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) { setTexParams(target, pname, params, NULL); }

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    // Max number of params considered is 4, for BORDER_COLOR.
    const GLfloat params[4] = { param, 0, 0, 0 };
    glTexParameterfv(target, pname, params);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    const GLint params[4] = { param, 0, 0, 0 };
    glTexParameteriv(target, pname, params);
}

static inline bool checkTexArgs(GLenum target, GLint level, GLsizei width, GLsizei height, GLint border) {
    if ((level < 0) || (level >= GLASS_NUM_TEX_LEVELS) || (border != 0))
        return false;

    if ((target != GL_TEXTURE_2D) && (width != height))
        return false;

    if ((width < GLASS_MIN_TEX_SIZE) || (height < GLASS_MIN_TEX_SIZE))
        return false;

    if ((width > GLASS_MAX_TEX_SIZE) || (height > GLASS_MAX_TEX_SIZE))
        return false;

    if (!kygxIsPo2(width) || !kygxIsPo2(height))
        return false;

    return true;
}

static inline bool isTexFormat(GLenum format) {
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

static inline bool isTexType(GLenum type) {
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

static inline GLenum texTargetForSubtarget(GLenum target) {
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

    KYGX_UNREACHABLE("Invalid parameter!");
}

static bool tryUnwrapTexFormat(GLenum format, GLenum type, GPUTexFormat* out) {
    if (format == GL_ALPHA) {
        if (type == GL_UNSIGNED_BYTE) {
            *out = TEXFORMAT_A8;
            return true;
        }

        if (type == GL_UNSIGNED_NIBBLE_PICA) {
            *out = TEXFORMAT_A4;
            return true;
        }

        return false;
    }

    if (format == GL_LUMINANCE) {
        if (type == GL_UNSIGNED_BYTE) {
            *out = TEXFORMAT_L8;
            return true;
        }

        if (type == GL_UNSIGNED_NIBBLE_PICA) {
            *out = TEXFORMAT_L4;
            return true;
        }

        return false;
    }

    if (format == GL_LUMINANCE_ALPHA) {
        if (type == GL_UNSIGNED_BYTE) {
            *out = TEXFORMAT_LA8;
            return true;
        }

        if (type == GL_UNSIGNED_BYTE_4_4_PICA) {
            *out = TEXFORMAT_LA4;
            return true;
        }

        return false;
    }

    if (format == GL_RGB) {
        if (type == GL_UNSIGNED_BYTE) {
            *out = TEXFORMAT_RGB8;
            return true;
        }

        if (type == GL_UNSIGNED_SHORT_5_6_5) {
            *out = TEXFORMAT_RGB565;
            return true;
        }

        return false;
    }

    if (format == GL_RGBA) {
        if (type == GL_UNSIGNED_BYTE) {
            *out = TEXFORMAT_RGBA8;
            return true;
        }

        if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
            *out = TEXFORMAT_RGB5A1;
            return true;
        }

        if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
            *out = TEXFORMAT_RGBA4;
            return true;
        }

        return false;
    }

    if (format == GL_HILO8_PICA && type == GL_UNSIGNED_BYTE) {
        *out = TEXFORMAT_HILO8;
        return true;
    }

    if (format == GL_ETC1_RGB8_OES) {
        *out = TEXFORMAT_ETC1;
        return true;
    }

    if (format == GL_ETC1_ALPHA_RGB8_A4_PICA) {
        *out = TEXFORMAT_ETC1A4;
        return true;
    }

    return false;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data) {
    if (!checkTexArgs(target, level, width, height, border)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }
    
    if (!isTexFormat(format) || !isTexType(type)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    GPUTexFormat nativeFormat;
    if (format != internalformat || !tryUnwrapTexFormat(format, type, &nativeFormat)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    // We don't support default textures.
    if (!tex) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Target check.
    if (tex->target != texTargetForSubtarget(target)) {
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
    TexReallocStatus reallocStatus = GLASS_tex_realloc(tex, width << level, height << level, nativeFormat, tex->vram);
    if (reallocStatus == TEXREALLOCSTATUS_FAILED)
        return;

    // Write data.
    if (data) {
        GLASS_tex_writeUntiled(tex, data, face, level);
        reallocStatus = TEXREALLOCSTATUS_UPDATED;
    }
    
    if (reallocStatus == TEXREALLOCSTATUS_UPDATED)
        ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;   
}

void glTexVRAMPICA(GLboolean enabled) {
    CtxCommon* ctx = GLASS_context_getBound();
    TextureInfo* tex = (TextureInfo*)ctx->textureUnits[ctx->activeTextureUnit];

    if (!tex) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    const TexReallocStatus reallocStatus = GLASS_tex_realloc(tex, tex->width, tex->height, tex->format, enabled);
    if (reallocStatus == TEXREALLOCSTATUS_UPDATED) {
        tex->vram = enabled;
        ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;
    }
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {
    GLASS_context_setError(GL_INVALID_ENUM);
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data) {
    GLASS_context_setError(GL_INVALID_ENUM);    
}

// TODO
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);