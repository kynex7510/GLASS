#include "Context.h"

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name); // TODO
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name); // TODO
GLint glGetAttribLocation(GLuint program, const GLchar* name); // TODO

void glDisableVertexAttribArray(GLuint index) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    AttributeInfo* attrib = &ctx->attribs[index];

    if (attrib->flags & ATTRIB_FLAG_ENABLED) {
        --ctx->numEnabledAttribs;
        attrib->flags &= ~(ATTRIB_FLAG_ENABLED);
        ctx->flags |= CONTEXT_FLAG_ATTRIBS;
    }
}

void glEnableVertexAttribArray(GLuint index) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    AttributeInfo* attrib = &ctx->attribs[index];

    if (!(attrib->flags & ATTRIB_FLAG_ENABLED)) {
        if (ctx->numEnabledAttribs >= GLASS_MAX_ENABLED_ATTRIBS) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        ++ctx->numEnabledAttribs;
        attrib->flags |= ATTRIB_FLAG_ENABLED;
        ctx->flags |= CONTEXT_FLAG_ATTRIBS;
    }
}

static bool GLASS_readFloats(size_t index, GLenum pname, GLfloat* params) {
    CtxCommon* ctx = GLASS_context_getCommon();
    const AttributeInfo* attrib = &ctx->attribs[index];

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (size_t i = 0; i < 4; ++i) {
            params[i] = attrib->components[i];
        }
        
        return true;
    }
    
    return false;
}

static bool GLASS_readInt(size_t index, GLenum pname, GLint* param) {
    CtxCommon* ctx = GLASS_context_getCommon();
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
            *param = (attrib->flags & ATTRIB_FLAG_ENABLED) ? GL_TRUE : GL_FALSE;
            return true;
    }

    return false;
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) {
    if (index >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    if (!GLASS_readFloats(index, pname, params)) {
        GLint param;

        if (!GLASS_readInt(index, pname, &param)) {
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

    if (!GLASS_readInt(index, pname, params)) {
        GLfloat components[4];

        if (!GLASS_readFloats(index, pname, components)) {
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

    CtxCommon* ctx = GLASS_context_getCommon();
    AttributeInfo* attrib = &ctx->attribs[index];

    // Recover virtual address.
    GLvoid* virtAddr = NULL;

    if (!(attrib->flags & ATTRIB_FLAG_FIXED)) {
        if (attrib->boundBuffer != GLASS_INVALID_OBJECT) {
            virtAddr = (GLvoid*)attrib->physOffset;
        } else {
            virtAddr = GLASS_utility_convertPhysToVirt(attrib->physAddr);
            ASSERT(virtAddr);
        }
    }

    *pointer = virtAddr;
}

static void GLASS_setFixedAttrib(GLuint reg, const GLfloat* params) {
    if (reg >= GLASS_NUM_ATTRIB_REGS) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    AttributeInfo* attrib = &ctx->attribs[reg];

    // Set fixed attribute.
    attrib->type = GL_FLOAT;
    attrib->count = 4;
    attrib->stride = sizeof(GLfloat) * 4;
    attrib->boundBuffer = GLASS_INVALID_OBJECT;
    attrib->physAddr = 0;
    attrib->physOffset = 0;
    attrib->flags |= ATTRIB_FLAG_FIXED;

    for (size_t i = 0; i < 4; ++i)
        attrib->components[i] = params[i];

    ctx->flags |= CONTEXT_FLAG_ATTRIBS;
}

void glVertexAttrib1f(GLuint index, GLfloat v0) {
    const GLfloat values[4] = {v0, 0.0f, 0.0f, 1.0f};
    GLASS_setFixedAttrib(index, values);
}

void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1) {
    const GLfloat values[4] = {v0, v1, 0.0f, 1.0f};
    GLASS_setFixedAttrib(index, values);
}

void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2) {
    const GLfloat values[4] = {v0, v1, v2, 1.0f};
    GLASS_setFixedAttrib(index, values);
}

void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    const GLfloat values[4] = {v0, v1, v2, v3};
    GLASS_setFixedAttrib(index, values);
}

void glVertexAttrib1fv(GLuint index, const GLfloat* v) {
    ASSERT(v);
    glVertexAttrib1f(index, v[0]);
}

void glVertexAttrib2fv(GLuint index, const GLfloat* v) {
    ASSERT(v);
    glVertexAttrib2f(index, v[0], v[1]);
}

void glVertexAttrib3fv(GLuint index, const GLfloat* v) {
    ASSERT(v);
    glVertexAttrib3f(index, v[0], v[1], v[2]);
}

void glVertexAttrib4fv(GLuint index, const GLfloat* v) {
    ASSERT(v);
    glVertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT && type != GL_FLOAT) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if (index >= GLASS_NUM_ATTRIB_REGS || size < 1 || size > 4 || stride < 0 || normalized != GL_FALSE) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getCommon();
    AttributeInfo* attrib = &ctx->attribs[index];

    // Get vertex buffer physical address.
    u32 physAddr = 0;
    u32 physOffset = 0;
    if (ctx->arrayBuffer != GLASS_INVALID_OBJECT) {
        const BufferInfo* binfo = (BufferInfo*)ctx->arrayBuffer;
        physAddr = osConvertVirtToPhys((void*)binfo->address);
        physOffset = (u32)pointer;
    } else {
        physAddr = osConvertVirtToPhys(pointer);
    }

    ASSERT(physAddr);

    // Set attribute values.
    attrib->type = type;
    attrib->count = size;
    attrib->stride = stride;
    attrib->boundBuffer = ctx->arrayBuffer;
    attrib->physAddr = physAddr;
    attrib->physOffset = physOffset;
    attrib->components[0] = 0.0f;
    attrib->components[1] = 0.0f;
    attrib->components[2] = 0.0f;
    attrib->components[3] = 1.0f;
    attrib->flags &= ~(ATTRIB_FLAG_FIXED);

    ctx->flags |= CONTEXT_FLAG_ATTRIBS;
}