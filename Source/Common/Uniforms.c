#include "Base/Context.h"
#include "Base/Utility.h"

#include <stdlib.h> // atoi
#include <string.h> // strstr, strlen, strncpy, memcpy

static bool GLASS_getUniformLocInfo(GLint loc, size_t* index, size_t* offset, bool* isGeometry) {
    ASSERT(index);
    ASSERT(offset);
    ASSERT(isGeometry);

    if (loc != -1) {
        *index = (loc >> 8) & 0xFF;
        *offset = loc & 0xFF;
        *isGeometry = (loc >> 16) & 1;
        return true;
    }

    return false;
}

static UniformInfo* GLASS_getShaderUniform(const ProgramInfo* program, size_t index, bool isGeometry) {
    ASSERT(program);

    ShaderInfo* shader = NULL;
    if (isGeometry) {
        if (!GLASS_OBJ_IS_SHADER(program->linkedGeometry)) {
            return NULL;
        }

        shader = (ShaderInfo*)program->linkedGeometry;
    } else {
        if (!GLASS_OBJ_IS_SHADER(program->linkedVertex)) {
            return NULL;
        }

        shader = (ShaderInfo*)program->linkedVertex;
    }

    if (index > shader->numOfActiveUniforms)
        return NULL;

    return &shader->activeUniforms[index];
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    ASSERT(size);
    ASSERT(type);
    ASSERT(name);

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
    ASSERT(shad);

    if (index > shad->numOfActiveUniforms) {
        index -= shad->numOfActiveUniforms;
        shad = (ShaderInfo*)prog->linkedGeometry;
    }

    if (!shad || (index > shad->numOfActiveUniforms)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Get uniform data.
    const UniformInfo* uni = &shad->activeUniforms[index];
    const size_t symLen = MIN(bufSize, strlen(uni->symbol));

    if (length)
        *length = symLen;

    *size = uni->count;
    strncpy(name, uni->symbol, symLen);
    name[symLen] = '\0';

    switch (uni->type) {
        case GLASS_UNI_BOOL:
            *type = GL_BOOL;
            break;
        case GLASS_UNI_INT:
            *type = GL_INT_VEC4;
            break;
        case GLASS_UNI_FLOAT:
            *type = GL_FLOAT_VEC4;
            break;
        default:
            UNREACHABLE("Invalid uniform type!");
    }
}

static bool GLASS_getBoolUniform(const UniformInfo* info, size_t offset) {
    ASSERT(info);
    ASSERT(info->type == GLASS_UNI_BOOL);
    ASSERT(info->count <= GLASS_NUM_BOOL_UNIFORMS);
    ASSERT(offset < info->count);

    return (info->data.mask >> offset) & 1;
}

static void GLASS_getIntUniform(const UniformInfo* info, size_t offset, u32* out) {
    ASSERT(info);
    ASSERT(out);
    ASSERT(info->type == GLASS_UNI_INT);
    ASSERT(info->count <= GLASS_NUM_INT_UNIFORMS);
    ASSERT(offset < info->count);

    *out = (info->count == 1) ? info->data.value : info->data.values[offset];
}

static void GLASS_getFloatUniform(const UniformInfo* info, size_t offset, u32* out) {
    ASSERT(info);
    ASSERT(out);
    ASSERT(info->type == GLASS_UNI_FLOAT);
    ASSERT(info->count <= GLASS_NUM_FLOAT_UNIFORMS);
    ASSERT(offset < info->count);
    memcpy(out, &info->data.values[3 * offset], 3 * sizeof(u32));
}

static void GLASS_getUniformValues(GLuint program, GLint location, GLint* intParams, GLfloat* floatParams) {
    ASSERT(intParams || floatParams);

    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* prog = (ProgramInfo*)program;

    // Parse location.
    size_t locIndex = 0;
    size_t locOffset = 0;
    bool locIsGeometry = 0;
    if (!GLASS_getUniformLocInfo(location, &locIndex, &locOffset, &locIsGeometry)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Get uniform.
    UniformInfo* uni = GLASS_getShaderUniform(prog, locIndex, locIsGeometry);
    if (!uni) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Handle bool.
    if (uni->type == GLASS_UNI_BOOL) {
        if (intParams) {
            ASSERT(!floatParams);
            intParams[0] = GLASS_getBoolUniform(uni, locOffset) ? 1 : 0;
        }

        if (floatParams)
            floatParams[0] = GLASS_getBoolUniform(uni, locOffset) ? 1.0f : 0.0f;

        return;
    }

    // Handle int.
    if (uni->type == GLASS_UNI_INT) {
        u32 packed = 0;
        u32 components[4];
        GLASS_getIntUniform(uni, locOffset, &packed);
        GLASS_utility_unpackIntVector(packed, components);

        if (intParams) {
            ASSERT(!floatParams);
            for (size_t i = 0; i < 4; ++i)
                intParams[i] = (GLint)components[i];
        }

        if (floatParams) {
            for (size_t i = 0; i < 4; ++i)
                floatParams[i] = (GLfloat)components[i];
        }

        return;
    }

    // Handle float.
    if (uni->type == GLASS_UNI_FLOAT) {
        u32 packed[3];
        GLASS_getFloatUniform(uni, locOffset, packed);

        if (floatParams) {
            ASSERT(!intParams);
            GLASS_utility_unpackFloatVector(packed, floatParams);
        }

        if (intParams) {
            float components[4];
            GLASS_utility_unpackFloatVector(packed, components);

            for (size_t i = 0; i < 4; ++i)
                intParams[i] = (GLint)components[i];
        }

        return;
    }

    UNREACHABLE("Invalid uniform type!");
}

void glGetUniformfv(GLuint program, GLint location, GLfloat* params) { GLASS_getUniformValues(program, location, NULL, params); }
void glGetUniformiv(GLuint program, GLint location, GLint* params) { GLASS_getUniformValues(program, location, params, NULL); }

static size_t GLASS_extractUniformOffset(const char* name) {
    ASSERT(name);

    if (strstr(name, ".") || (strstr(name, "gl_") == name))
        return -1;

    const char* beg = strstr(name, "[");
    if (!beg)
        return 0;

    const char* end = strstr(beg, "]");
    if (!end || (end[1] != '\0') || ((size_t)(end - beg) == 0))
        return -1;

    return atoi(&beg[1]);
}

static bool GLASS_checkUniformOffset(u8 type, size_t offset) {
    switch (type) {
        case GLASS_UNI_BOOL:
            return offset < GLASS_NUM_BOOL_UNIFORMS;
        case GLASS_UNI_INT:
            return offset < GLASS_NUM_INT_UNIFORMS;
        case GLASS_UNI_FLOAT:
            return offset < GLASS_NUM_FLOAT_UNIFORMS;
    }

    UNREACHABLE("Invalid uniform type!");
}

static GLint GLASS_lookupUniform(const ShaderInfo* shader, const char* name, size_t offset) {
    for (size_t i = 0; i < shader->numOfActiveUniforms; ++i) {
        const UniformInfo* uni = &shader->activeUniforms[i];

        if (strstr(name, uni->symbol) != name)
            continue;

        if (!GLASS_checkUniformOffset(uni->type, offset) || (offset > uni->count))
            break;

        // Make location.
        return (GLint)(((size_t)(shader->flags & GLASS_SHADER_FLAG_GEOMETRY) << 16) | (i << 8) | (offset & 0xFF));
    }

    return -1;
}

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    ProgramInfo* prog = (ProgramInfo*)program;
    if (prog->flags & GLASS_PROGRAM_FLAG_LINK_FAILED) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    // Get offset.
    const size_t offset = GLASS_extractUniformOffset(name);
    if (offset == -1)
        return -1;

    if (offset != -1) {
        // Lookup uniform.
        if (GLASS_OBJ_IS_SHADER(prog->linkedVertex)) {
            ShaderInfo* vshad = (ShaderInfo*)prog->linkedVertex;
            ShaderInfo* gshad = (ShaderInfo*)prog->linkedGeometry;
            GLint loc = GLASS_lookupUniform(vshad, name, offset);

            if ((loc == -1) && gshad)
                loc = GLASS_lookupUniform(gshad, name, offset);

            return loc;
        }
    }

    return -1;
}

static void GLASS_setBoolUniform(UniformInfo* info, size_t offset, bool enabled) {
    ASSERT(info);
    ASSERT(info->type == GLASS_UNI_BOOL);
    ASSERT(info->count <= GLASS_NUM_BOOL_UNIFORMS);
    ASSERT(offset < info->count);

    if (enabled) {
        info->data.mask |= (1u << offset);
    } else {
        info->data.mask &= ~(1u << offset);
    }

    info->dirty = true;
}

static void GLASS_setIntUniform(UniformInfo* info, size_t offset, u32 vector) {
    ASSERT(info);
    ASSERT(info->type == GLASS_UNI_INT);
    ASSERT(info->count <= GLASS_NUM_INT_UNIFORMS);
    ASSERT(offset < info->count);

    if (info->count == 1) {
        info->data.value = vector;
    } else {
        info->data.values[offset] = vector;
    }

    info->dirty = true;
}

static void GLASS_setFloatUniform(UniformInfo* info, size_t offset, const u32* vector) {
    ASSERT(info);
    ASSERT(info->type == GLASS_UNI_FLOAT);
    ASSERT(info->count <= GLASS_NUM_FLOAT_UNIFORMS);
    ASSERT(offset < info->count);

    memcpy(&info->data.values[3 * offset], vector, 3 * sizeof(u32));
    info->dirty = true;
}

static void GLASS_setUniformValues(GLint location, const GLint* intValues, const GLfloat* floatValues, size_t numOfComponents, GLsizei numOfElements) {
    ASSERT(numOfComponents <= 4);

    if (numOfElements < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Parse location.
    size_t locIndex = 0;
    size_t locOffset = 0;
    bool locIsGeometry = 0;
    if (!GLASS_getUniformLocInfo(location, &locIndex, &locOffset, &locIsGeometry))
        return; // A value of -1 is silently ignored.

    // Get program.
    CtxCommon* ctx = GLASS_context_getCommon();
    if (!GLASS_OBJ_IS_PROGRAM(ctx->currentProgram)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* prog = (ProgramInfo*)ctx->currentProgram;

    // Get uniform.
    UniformInfo* uni = GLASS_getShaderUniform(prog, locIndex, locIsGeometry);
    if (!uni || (locOffset >= uni->count) || (uni->count == 1 && numOfElements != 1)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    // Handle bool.
    if (uni->type == GLASS_UNI_BOOL) {
        if (numOfComponents != 1) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        for (size_t i = locOffset; i < MIN(uni->count, locOffset + numOfElements); ++i) {
            if (intValues) {
                ASSERT(!floatValues);
                GLASS_setBoolUniform(uni, i, intValues[i] != 0);
            } else if (floatValues) {
                GLASS_setBoolUniform(uni, i, floatValues[i] != 0.0f);
            } else {
                UNREACHABLE("Value buffer was nullptr!");
            }
        }

        return;
    }

    // Handle int.
    if (uni->type == GLASS_UNI_INT) {
        if (!intValues) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        for (size_t i = locOffset; i < MIN(uni->count, locOffset + numOfElements); ++i) {
            u32 components[4];
            u32 packed = 0;

            GLASS_getIntUniform(uni, i, &packed);
            GLASS_utility_unpackIntVector(packed, components);

            for (size_t j = 0; j < numOfComponents; ++j)
                components[j] = intValues[(numOfComponents * (i - locOffset)) + j];

            GLASS_utility_packIntVector(components, &packed);
            GLASS_setIntUniform(uni, i, packed);
        }

        return;
    }

    // Handle float.
    if (uni->type == GLASS_UNI_FLOAT) {
        if (!floatValues) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        for (size_t i = locOffset; i < MIN(uni->count, locOffset + numOfElements); ++i) {
            float components[4];
            u32 packed[3];

            GLASS_getFloatUniform(uni, i, packed);
            GLASS_utility_unpackFloatVector(packed, components);

            for (size_t j = 0; j < numOfComponents; ++j)
                components[j] = floatValues[(numOfComponents * (i - locOffset)) + j];

            GLASS_utility_packFloatVector(components, packed);
            GLASS_setFloatUniform(uni, i, packed);
        }

        return;
    }

    UNREACHABLE("Invalid uniform type!");
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) { GLASS_setUniformValues(location, NULL, value, 1, count); }
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) { GLASS_setUniformValues(location, NULL, value, 2, count); }
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) { GLASS_setUniformValues(location, NULL, value, 3, count); }
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) { GLASS_setUniformValues(location, NULL, value, 4, count); }
void glUniform1iv(GLint location, GLsizei count, const GLint* value) { GLASS_setUniformValues(location, value, NULL, 1, count); }
void glUniform2iv(GLint location, GLsizei count, const GLint* value) { GLASS_setUniformValues(location, value, NULL, 2, count); }
void glUniform3iv(GLint location, GLsizei count, const GLint* value) { GLASS_setUniformValues(location, value, NULL, 3, count); }
void glUniform4iv(GLint location, GLsizei count, const GLint* value) { GLASS_setUniformValues(location, value, NULL, 4, count); }

void glUniform1f(GLint location, GLfloat v0) {
    GLfloat values[] = {v0};
    glUniform1fv(location, 1, values);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GLfloat values[] = {v0, v1};
    glUniform2fv(location, 1, values);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLfloat values[] = {v0, v1, v2};
    glUniform3fv(location, 1, values);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLfloat values[] = {v0, v1, v2, v3};
    glUniform4fv(location, 1, values);
}

void glUniform1i(GLint location, GLint v0) {
    GLint values[] = {v0};
    glUniform1iv(location, 1, values);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
    GLint values[] = {v0, v1};
    glUniform2iv(location, 1, values);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GLint values[] = {v0, v1, v2};
    glUniform3iv(location, 1, values);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLint values[] = {v0, v1, v2, v3};
    glUniform4iv(location, 1, values);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (transpose != GL_FALSE) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    glUniform2fv(location, 2 * count, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (transpose != GL_FALSE) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    glUniform3fv(location, 3 * count, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (transpose != GL_FALSE) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    glUniform4fv(location, 4 * count, value);
}