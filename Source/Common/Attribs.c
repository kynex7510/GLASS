/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Wrappers/FlushCacheRegions.h>
#include <KYGX/Utility.h>

#include "Base/Context.h"
#include "Base/Math.h"

#include <string.h> // strncpy, strcmp

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    GLASS_context_setError(GL_INVALID_OPERATION);
    return;
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    const ProgramInfo* prog = (const ProgramInfo*)program;

    if (prog->flags & GLASS_PROGRAM_FLAG_LINK_FAILED) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    const ShaderInfo* shaders[] = {
        (ShaderInfo*)prog->linkedVertex,
        (ShaderInfo*)prog->linkedGeometry
    };

    for (size_t i = 0; i < 2; ++i) {
        const ShaderInfo* shader = shaders[i];
        if (!shader)
            continue;

        for (size_t j = 0; j < shader->numOfActiveAttribs; ++j) {
            const ActiveAttribInfo* attrib = &shader->activeAttribs[j];
            if (strcmp(name, attrib->symbol) == 0)
                return attrib->ID;
        }
    }

    return -1;
}

void glDisableVertexAttribArray(GLuint index) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    AttributeInfo* attrib = &ctx->attribs[index];

    if (attrib->flags & GLASS_ATTRIB_FLAG_ENABLED) {
        --ctx->numEnabledAttribs;
        attrib->flags &= ~(GLASS_ATTRIB_FLAG_ENABLED);
        ctx->flags |= GLASS_CONTEXT_FLAG_ATTRIBS;
    }
}

void glEnableVertexAttribArray(GLuint index) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    AttributeInfo* attrib = &ctx->attribs[index];

    if (!(attrib->flags & GLASS_ATTRIB_FLAG_ENABLED)) {
        if (ctx->numEnabledAttribs >= GLASS_MAX_ENABLED_ATTRIBS) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        ++ctx->numEnabledAttribs;
        attrib->flags |= GLASS_ATTRIB_FLAG_ENABLED;
        ctx->flags |= GLASS_CONTEXT_FLAG_ATTRIBS;
    }
}

static bool readFloats(size_t index, GLenum pname, GLfloat* params) {
    CtxCommon* ctx = GLASS_context_getBound();
    const AttributeInfo* attrib = &ctx->attribs[index];

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (size_t i = 0; i < 4; ++i) {
            params[i] = attrib->components[i];
        }
        
        return true;
    }
    
    return false;
}

static bool readInt(size_t index, GLenum pname, GLint* param) {
    CtxCommon* ctx = GLASS_context_getBound();
    const AttributeInfo* attrib = &ctx->attribs[index];

    switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *param = attrib->boundBuffer;
            return true;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *param = attrib->count;
            return true;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *param = attrib->stride;
            return true;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *param = attrib->type;
            return true;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *param = GL_FALSE;
            return true;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *param = ((attrib->flags & GLASS_ATTRIB_FLAG_ENABLED) ? GL_TRUE : GL_FALSE);
            return true;
    }

    return false;
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!readFloats(index, pname, params)) {
        GLint param;

        if (!readInt(index, pname, &param)) {
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
        }

        params[0] = (GLfloat)param;
    }
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!readInt(index, pname, params)) {
        GLfloat components[4];

        if (!readFloats(index, pname, components)) {
            GLASS_context_setError(GL_INVALID_ENUM);
            return;
        }

        for (size_t i = 0; i < 4; ++i)
            params[i] = (GLint)components[i];
    }
}

void glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    AttributeInfo* attrib = &ctx->attribs[index];

    // Get virtual address.
    GLvoid* virtAddr = NULL;

    if (!(attrib->flags & GLASS_ATTRIB_FLAG_FIXED)) {
        if (attrib->boundBuffer != GLASS_INVALID_OBJECT) {
            virtAddr = (GLvoid*)attrib->bufferOffset;
        } else {
            virtAddr = kygxGetVirtualAddress(attrib->physAddr);
            KYGX_ASSERT(virtAddr);
        }
    }

    *pointer = virtAddr;
}

static void setFixedAttrib(GLuint reg, const GLfloat* params) {
    if (reg >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    AttributeInfo* attrib = &ctx->attribs[reg];

    // Set fixed attribute.
    attrib->type = GL_FLOAT;
    attrib->count = 4;
    attrib->stride = sizeof(GLfloat) * 4;
    attrib->boundBuffer = GLASS_INVALID_OBJECT;
    attrib->physAddr = 0;
    attrib->bufferOffset = 0;
    attrib->bufferSize = 0;
    attrib->sizeOfPrePad = 0;
    attrib->sizeOfPostPad = 0;
    attrib->flags |= GLASS_ATTRIB_FLAG_FIXED;

    for (size_t i = 0; i < 4; ++i)
        attrib->components[i] = params[i];

    ctx->flags |= GLASS_CONTEXT_FLAG_ATTRIBS;
}

void glVertexAttrib1f(GLuint index, GLfloat v0) {
    const GLfloat values[4] = {v0, 0.0f, 0.0f, 1.0f};
    setFixedAttrib(index, values);
}

void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1) {
    const GLfloat values[4] = {v0, v1, 0.0f, 1.0f};
    setFixedAttrib(index, values);
}

void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2) {
    const GLfloat values[4] = {v0, v1, v2, 1.0f};
    setFixedAttrib(index, values);
}

void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    const GLfloat values[4] = {v0, v1, v2, v3};
    setFixedAttrib(index, values);
}

void glVertexAttrib1fv(GLuint index, const GLfloat* v) {
    KYGX_ASSERT(v);
    glVertexAttrib1f(index, v[0]);
}

void glVertexAttrib2fv(GLuint index, const GLfloat* v) {
    KYGX_ASSERT(v);
    glVertexAttrib2f(index, v[0], v[1]);
}

void glVertexAttrib3fv(GLuint index, const GLfloat* v) {
    KYGX_ASSERT(v);
    glVertexAttrib3f(index, v[0], v[1], v[2]);
}

void glVertexAttrib4fv(GLuint index, const GLfloat* v) {
    KYGX_ASSERT(v);
    glVertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

static inline size_t sizeForAttribType(GLenum type) {
    switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return 1;
        case GL_SHORT:
            return 2;
        case GL_FLOAT:
            return 4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline bool isAttribPhysAddrAligned(GLenum type, u32 physAddr) {
    if (type == GL_SHORT)
        return (physAddr & 0x01) == 0;

    if (type == GL_FLOAT)
        return (physAddr & 0x03) == 0;

    return true;
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
    if ((type != GL_BYTE) && (type != GL_UNSIGNED_BYTE) && (type != GL_SHORT) && (type != GL_FLOAT)) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if ((index >= GLASS_NUM_ATTRIB_REGS) || (size < 1) || (size > 4) || (stride < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (normalized != GL_FALSE) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    AttributeInfo* attrib = &ctx->attribs[index];

    // Calculate buffer size.
    const size_t componentDataSize = size * sizeForAttribType(type);
    const size_t bufferSize = (stride ? stride : componentDataSize);

    // Get vertex buffer physical address.
    u32 physAddr = 0;
    size_t bufferOffset = 0;
    if (ctx->arrayBuffer != GLASS_INVALID_OBJECT) {
        const BufferInfo* binfo = (BufferInfo*)ctx->arrayBuffer;
        physAddr = kygxGetPhysicalAddress((void*)binfo->address);
        KYGX_ASSERT(physAddr);
        bufferOffset = (size_t)pointer;
    } else {
        physAddr = kygxGetPhysicalAddress(pointer);
        if (!physAddr) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        if (!ctx->params.flushAllLinearMem)
            kygxSyncFlushSingleBuffer(pointer, bufferSize);
    }

    // Check alignment.
    if (!isAttribPhysAddrAligned(type, physAddr + bufferOffset)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Calculate padding.
    if (bufferSize > componentDataSize) {
        // If we have a bound buffer we might need padding at the start.
        if ((ctx->arrayBuffer != GLASS_INVALID_OBJECT)) {
            attrib->sizeOfPrePad = (size_t)pointer;
        } else {
            attrib->sizeOfPrePad = 0;
        }

        // If the component data is not at the end of the buffer, we need to add pads.
        attrib->sizeOfPostPad = bufferSize - (attrib->sizeOfPrePad + componentDataSize);
    }

    // Set attribute values.
    attrib->type = type;
    attrib->count = size;
    attrib->stride = stride;
    attrib->boundBuffer = ctx->arrayBuffer;
    attrib->physAddr = physAddr;
    attrib->bufferOffset = bufferOffset;
    attrib->bufferSize = bufferSize;
    attrib->components[0] = 0.0f;
    attrib->components[1] = 0.0f;
    attrib->components[2] = 0.0f;
    attrib->components[3] = 1.0f;
    attrib->flags &= ~(GLASS_ATTRIB_FLAG_FIXED);

    ctx->flags |= GLASS_CONTEXT_FLAG_ATTRIBS;
}

// TODO: this function looks fishy.
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    KYGX_ASSERT(size);
    KYGX_ASSERT(type);
    KYGX_ASSERT(name);

    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (bufSize < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    ProgramInfo* prog = (ProgramInfo*)program;
    if (prog->flags & GLASS_PROGRAM_FLAG_LINK_FAILED) {
        if (length)
            *length = 0;

        if (bufSize)
            name[bufSize - 1] = '\0';

        return;
    }

    // Get shader.
    ShaderInfo* shad = (ShaderInfo*)prog->linkedVertex;
    KYGX_ASSERT(shad);

    if (index > shad->numOfActiveUniforms) {
        index -= shad->numOfActiveUniforms;
        shad = (ShaderInfo*)prog->linkedGeometry;
    }

    if (!shad || (index > shad->numOfActiveUniforms)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Get attribute data.
    const ActiveAttribInfo* attrib = &shad->activeAttribs[index];
    const size_t symLen = GLASS_MIN(bufSize, strlen(attrib->symbol));

    if (length)
        *length = symLen;

    *size = 1;
    strncpy(name, attrib->symbol, symLen);
    name[symLen] = '\0';
    *type = GL_FLOAT_VEC4;
}