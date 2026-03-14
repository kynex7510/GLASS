/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Base/Context.h"
#include "Base/Math.h"

#define TYPE_BOOL 0
#define TYPE_FLOAT 1
#define TYPE_INT 2

#define NORMALIZE_INT_CONSTANT (GLfloat)((1u << (sizeof(GLint) * 8 - 1)) - 1)

typedef struct {
    size_t type;
    void* p;
    size_t numParams;
    bool normalizeInt; // Normalize int between -MAX_INT...MAX_INT, only meaningful for floats.
} Value;

static void getAsBool(const Value* in, GLboolean* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(in->p);
    KYGX_ASSERT(out);

    if (in->type == TYPE_BOOL) {
        memcpy(out, in->p, in->numParams * sizeof(GLboolean));
        return;
    }

    if (in->type == TYPE_FLOAT) {
        const GLfloat* src = (GLfloat*)in->p;

        for (size_t i = 0; i < in->numParams; ++i)
            out[i] = src[i] == 0.0 ? GL_FALSE : GL_TRUE;

        return;
    }

    if (in->type == TYPE_INT) {
        const GLint* src = (GLint*)in->p;

        for (size_t i = 0; i < in->numParams; ++i)
            out[i] = src[i] == 0 ? GL_FALSE : GL_TRUE;

        return;
    }
}

static void getAsFloat(const Value* in, GLfloat* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(in->p);
    KYGX_ASSERT(out);

    if (in->type == TYPE_FLOAT) {
        memcpy(out, in->p, in->numParams * sizeof(GLfloat));
        return;
    }

    if (in->type == TYPE_BOOL) {
        const GLboolean* src = (GLboolean*)in->p;

        for (size_t i = 0; i < in->numParams; ++i)
            out[i] = src[i] == GL_FALSE ? 0.0 : 1.0;

        return;
    }

    if (in->type == TYPE_INT) {
        const GLint* src = (GLint*)in->p;

        for (size_t i = 0; i < in->numParams; ++i)
            out[i] = src[i];

        return;
    }
}

static void getAsInt(const Value* in, GLint* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(in->p);
    KYGX_ASSERT(out);

    if (in->type == TYPE_INT) {
        memcpy(out, in->p, in->numParams * sizeof(GLint));
        return;
    }

    if (in->type == TYPE_BOOL) {
        const GLboolean* src = (GLboolean*)in->p;

        for (size_t i = 0; i < in->numParams; ++i)
            out[i] = src[i] == GL_FALSE ? 0 : 1;

        return;
    }

    if (in->type == TYPE_FLOAT) {
        const GLfloat* src = (GLfloat*)in->p;

        for (size_t i = 0; i < in->numParams; ++i) {
            if (in->normalizeInt) {
                KYGX_ASSERT(GLASS_CLAMP(-1.0, 1.0, src[i]) == src[i]);
                out[i] = src[i] * NORMALIZE_INT_CONSTANT;
            } else {
                out[i] = GLASS_ROUND(src[i]);
            }
        }
    }
}

static void getImpl(GLenum pname, size_t type, void* out) {
    KYGX_ASSERT(out);

    GLboolean boolBuffer[1];
    GLfloat floatBuffer[4];
    GLint intBuffer[1];

    Value value;
    value.type = 0;
    value.p = NULL;
    value.numParams = 0;
    value.normalizeInt = false;

    CtxCommon* ctx = GLASS_context_getBound();

#define ON_GET(name) case name
#define END_CASE break;

#define SET_TYPE_BOOL       \
    value.type = TYPE_BOOL; \
    value.p = boolBuffer;

#define SET_TYPE_FLOAT       \
    value.type = TYPE_FLOAT; \
    value.p = floatBuffer;

#define SET_TYPE_INT       \
    value.type = TYPE_INT; \
    value.p = intBuffer;

#define SET_TYPE(t) SET_TYPE_##t

#define SET_NUM_PARAMS(n) value.numParams = n;

#define ENABLE_NORMALIZATION() value.normalizeInt = true;

#define SET_BOOL_PARAM(idx, v) boolBuffer[idx] = (v);
#define SET_FLOAT_PARAM(idx, v) floatBuffer[idx] = (v);
#define SET_INT_PARAM(idx, v) intBuffer[idx] = (v);

    switch (pname) {
#include "Common/Get.inc"
    }

#undef ON_GET
#undef END_CASE
#undef SET_TYPE_BOOL
#undef SET_TYPE_FLOAT
#undef SET_TYPE_INT
#undef SET_TYPE
#undef SET_NUM_PARAMS
#undef ENABLE_NORMALIZATION
#undef SET_BOOL_PARAM
#undef SET_FLOAT_PARAM
#undef SET_INT_PARAM

    if (value.numParams) {
        switch (type) {
            case TYPE_BOOL:
                getAsBool(&value, (GLboolean*)out);
                break;
            case TYPE_FLOAT:
                getAsFloat(&value, (GLfloat*)out);
                break;
            case TYPE_INT:
                getAsInt(&value, (GLint*)out);
                break;
        }
    } else {
        GLASS_context_setError(GL_INVALID_ENUM);
    }
}

void glGetBooleanv(GLenum pname, GLboolean* params) { getImpl(pname, TYPE_BOOL, params); }
void glGetFloatv(GLenum pname, GLfloat* params) { getImpl(pname, TYPE_FLOAT, params); }
void glGetIntegerv(GLenum pname, GLint* params) { getImpl(pname, TYPE_INT, params); }