#include "Base/Context.h"

#include <string.h> // memcpy

static BufferInfo* GLASS_getBoundBufferInfo(GLenum target) {
    GLuint buffer = GLASS_INVALID_OBJECT;
    CtxCommon* ctx = GLASS_context_getCommon();

    switch (target) {
        case GL_ARRAY_BUFFER:
            buffer = ctx->arrayBuffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            buffer = ctx->elementArrayBuffer;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return NULL;
    }

    if (GLASS_OBJ_IS_BUFFER(buffer))
        return (BufferInfo*)buffer;

    GLASS_context_setError(GL_INVALID_OPERATION);
    return NULL;
}

void glBindBuffer(GLenum target, GLuint buffer) {
    ASSERT(GLASS_OBJ_IS_BUFFER(buffer) || buffer == GLASS_INVALID_OBJECT);

    BufferInfo*info = (BufferInfo*)buffer;
    CtxCommon* ctx = GLASS_context_getCommon();

    // Bind buffer to context.
    switch (target) {
        case GL_ARRAY_BUFFER:
            ctx->arrayBuffer = buffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            ctx->elementArrayBuffer = buffer;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
    }

    if (info)
        info->bound = true;
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    if ((usage != GL_STREAM_DRAW) && (usage != GL_STATIC_DRAW) && (usage != GL_DYNAMIC_DRAW)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (size < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Get buffer.
    BufferInfo* info = GLASS_getBoundBufferInfo(target);
    if (!info)
        return;

    // Allocate buffer.
    if (info->address)
        glassLinearFree(info->address);

    info->address = glassLinearAlloc(size);
    if (!info->address) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        return;
    }

    info->usage = usage;

    if (data) {
        memcpy(info->address, data, size);

        CtxCommon* ctx = GLASS_context_getCommon();
        if (!ctx->initParams.flushAllLinearMem) {
            ASSERT(GLASS_utility_flushCache(info->address, size));
        }
    }
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    ASSERT(data);

    if ((offset < 0) || (size < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Get buffer.
    BufferInfo* info = GLASS_getBoundBufferInfo(target);
    if (!info)
        return;

    // Get buffer size.
    GLsizeiptr bufSize = (GLsizeiptr)glassLinearSize(info->address);
    if (size > (bufSize - offset)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Copy data.
    memcpy(info->address + offset, data, size);

    CtxCommon* ctx = GLASS_context_getCommon();
    if (!ctx->initParams.flushAllLinearMem) {
        ASSERT(GLASS_utility_flushCache(info->address + offset, size));
    }
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    ASSERT(buffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();

    for (size_t i = 0; i < n; ++i) {
        GLuint name = buffers[i];

        // Validate name.
        if (!GLASS_OBJ_IS_BUFFER(name))
            continue;

        BufferInfo* info = (BufferInfo*)name;

        // Unbind if bound.
        if (ctx->arrayBuffer == name)
            ctx->arrayBuffer = GLASS_INVALID_OBJECT;
        
        if (ctx->elementArrayBuffer == name)
            ctx->elementArrayBuffer = GLASS_INVALID_OBJECT;

        // Delete buffer.
        if (info->address)
            glassLinearFree(info->address);

        glassVirtualFree(info);
    }
}

void glGenBuffers(GLsizei n, GLuint* buffers) {
    ASSERT(buffers);

    if (n < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        GLuint name = GLASS_createObject(GLASS_BUFFER_TYPE);
        if (!GLASS_OBJ_IS_BUFFER(name)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            return;
        }

        BufferInfo* info = (BufferInfo*)name;
        info->usage = GL_STATIC_DRAW;
        buffers[i] = name;
    }
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* data) {
    ASSERT(data);

    BufferInfo* info = GLASS_getBoundBufferInfo(target);
    if (!info)
        return;

    switch (pname) {
        case GL_BUFFER_SIZE:
            *data = glassLinearSize(info->address);
            break;
        case GL_BUFFER_USAGE:
            *data = info->usage;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
    }
}

GLboolean glIsBuffer(GLuint buffer) {
    if (GLASS_OBJ_IS_BUFFER(buffer)) {
        const BufferInfo* info = (BufferInfo*)buffer;
        if (info->bound)
            return GL_TRUE;
    }

    return GL_FALSE;
}