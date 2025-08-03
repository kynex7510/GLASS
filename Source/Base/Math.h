/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _GLASS_BASE_MATH_H
#define _GLASS_BASE_MATH_H

#include <kazmath/kazmath.h>

#include "Base/Types.h"

#define GLASS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define GLASS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define GLASS_CLAMP(min, max, val) (GLASS_MAX((min), GLASS_MIN((max), (val))))

typedef struct {
    kmMat4* matrices;
    size_t count;
    size_t capacity;
} MtxStack;

u32 GLASS_math_f32tofixed13(float f);
u32 GLASS_math_f32tof31(float f);
u32 GLASS_math_f32tof24(float f);
float GLASS_math_f24tof32(u32 f);

void GLASS_math_packIntVector(const u32* in, u32* out);
void GLASS_math_unpackIntVector(u32 in, u32* out);

void GLASS_math_packFloatVector(const float* in, u32* out);
void GLASS_math_unpackFloatVector(const u32* in, float* out);

void GLASS_mtxstack_init(MtxStack* s, size_t capacity);
void GLASS_mtxstack_destroy(MtxStack* s);
bool GLASS_mtxstack_push(MtxStack* s);
bool GLASS_mtxstack_pop(MtxStack* s);
void GLASS_mtxstack_load(MtxStack* s, const GLfloat* mtx);
void GLASS_mtxstack_multiply(MtxStack* s, const GLfloat* mtx);

#endif /* _GLASS_BASE_MATH_H */