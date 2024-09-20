#include "Context.h"
#include "Utility.h"

#include <stdlib.h> // atoi
#include <string.h> // strstr, strlen, strncpy

#define CHECK_OFFSET(type, offset) \
    ((((type) == GLASS_UNI_BOOL) && ((offset) < GLASS_NUM_BOOL_UNIFORMS)) || (((type) == GLASS_UNI_INT) && ((offset) < GLASS_NUM_INT_UNIFORMS)) || (((type) == GLASS_UNI_FLOAT) && ((offset) < GLASS_NUM_FLOAT_UNIFORMS)))

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
        if (!OBJ_IS_SHADER(program->linkedGeometry)) {
            return NULL;
        }

        shader = (ShaderInfo*)program->linkedGeometry;
    } else {
        if (!OBJ_IS_SHADER(program->linkedVertex)) {
            return NULL;
        }

        shader = (ShaderInfo*)program->linkedVertex;
    }

    if (index > shader->numOfActiveUniforms)
        return NULL;

    return &shader->activeUniforms[index];
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    if (bufSize) {
        ASSERT(name);
    }

    if (!OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (bufSize < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    ProgramInfo* prog = (ProgramInfo*)program;

    // Get shader.
    ShaderInfo* shad = (ShaderInfo*)prog->linkedVertex;
    if (!shad)
        return;

    if (index > shad->numOfActiveUniforms) {
        index -= shad->numOfActiveUniforms;
        shad = (ShaderInfo*)prog->linkedGeometry;
    }

    if (!shad)
        return;

    if (index > shad->numOfActiveUniforms) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    // Get uniform data.
    const UniformInfo* uni = &shad->activeUniforms[index];
    size_t symLength = MIN(bufSize, strlen(uni->symbol));

    if (length)
        *length = symLength;

    *size = uni->count;
    strncpy(name, uni->symbol, symLength);

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

void GLASS_getUniformValues(GLuint program, GLint location, GLint* intParams, GLfloat* floatParams) {
    ASSERT(intParams || floatParams);

    if (!OBJ_IS_PROGRAM(program)) {
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
        if (intParams)
            intParams[0] = GLASS_utility_getBoolUniform(uni, locOffset) ? 1 : 0;

        if (floatParams)
            floatParams[0] = GLASS_utility_getBoolUniform(uni, locOffset) ? 1.0f : 0.0f;

        return;
    }

    // Handle int.
    if (uni->type == GLASS_UNI_INT) {
        u32 packed = 0;
        u32 components[4] = {};
        GLASS_utility_getIntUniform(uni, locOffset, &packed);
        GLASS_utility_unpackIntVector(packed, components);

        if (intParams) {
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
        u32 packed[3] = {};
        GLASS_utility_getFloatUniform(uni, locOffset, packed);

        if (floatParams)
            GLASS_utility_unpackFloatVector(packed, floatParams);

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
    if (!end || end[1] != '\0' || !(size_t)(end - beg))
        return -1;

    return atoi(&beg[1]);
}

static GLint GLASS_lookupUniform(const ShaderInfo* shader, const char* name, size_t offset) {
    for (size_t i = 0; i < shader->numOfActiveUniforms; ++i) {
        const UniformInfo* uni = &shader->activeUniforms[i];

        if (strstr(name, uni->symbol) != name)
            continue;

        if (!CHECK_OFFSET(uni->type, offset) || offset < uni->count)
            break;

        // Make location.
        return (GLint)(((size_t)(shader->flags & SHADER_FLAG_GEOMETRY) << 16) | (i << 8) | (offset & 0xFF));
    }

    return -1;
}

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    if (!OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    ProgramInfo* prog = (ProgramInfo*)program;
    if (prog->flags & PROGRAM_FLAG_LINK_FAILED) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return -1;
    }

    // Get offset.
    const size_t offset = GLASS_extractUniformOffset(name);
    if (offset == -1)
        return -1;

    if (offset != -1) {
        // Lookup uniform.
        if (OBJ_IS_SHADER(prog->linkedVertex)) {
            ShaderInfo* vshad = (ShaderInfo*)prog->linkedVertex;
            ShaderInfo* gshad = (ShaderInfo*)prog->linkedGeometry;
            GLint loc = GLASS_lookupUniform(vshad, name, offset);

            if (loc == -1 && gshad)
                loc = GLASS_lookupUniform(gshad, name, offset);

            return loc;
        }
    }

    return -1;
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
    if (!OBJ_IS_PROGRAM(ctx->currentProgram)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* prog = (ProgramInfo*)ctx->currentProgram;

    // Get uniform.
    UniformInfo* uni = GLASS_getShaderUniform(prog, locIndex, locIsGeometry);
    if (!uni || locOffset >= uni->count || (uni->count == 1 && numOfElements != 1)) {
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
                GLASS_utility_setBoolUniform(uni, i, intValues[i] != 0);
            } else if (floatValues) {
                GLASS_utility_setBoolUniform(uni, i, floatValues[i] != 0.0f);
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
            u32 components[4] = {};
            u32 packed = 0;

            GLASS_utility_getIntUniform(uni, i, &packed);
            GLASS_utility_unpackIntVector(packed, components);

            for (size_t j = 0; j < numOfComponents; ++j)
                components[j] = intValues[(numOfComponents * (i - locOffset)) + j];

            GLASS_utility_packIntVector(components, &packed);
            GLASS_utility_setIntUniform(uni, i, packed);
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
            float components[4] = {};
            u32 packed[3] = {};

            GLASS_utility_getFloatUniform(uni, i, packed);
            GLASS_utility_unpackFloatVector(packed, components);

            for (size_t j = 0; j < numOfComponents; ++j)
                components[j] = floatValues[(numOfComponents * (i - locOffset)) + j];

            GLASS_utility_packFloatVector(components, packed);
            GLASS_utility_setFloatUniform(uni, i, packed);
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