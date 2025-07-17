/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Base/Math.h"

static inline float makef(u32 bits) {
    union {
        float val;
        u32 bits;
    } cast;

    cast.bits = bits;
    return cast.val;
}

u32 GLASS_math_f32tofixed13(float f) {
    f = GLASS_CLAMP(-15.255, 15.255, f);
    u32 sign = 0;

    if (f < 0.0f) {
        f = -f;
        sign |= (1u << 12);
    }

    const u32 i = ((u32)(f) & 0xF);
    return (sign | (i << 8) | ((u32)((f - i) * 1000.0f) & 0xFF));
}

float GLASS_math_f24tof32(u32 f) {
    const u32 sign = f >> 23;
    u32 bits = 0;

    if (!(f & 0x7FFFFF)) {
        bits = (sign << 31);
    } else if (((f >> 16) & 0xFF) == 0x7F) {
        bits = (sign << 31) | (0xFF << 23);
    } else {
        const u32 mantissa = f & 0xFFFF;
        const s32 exponent = ((f >> 16) & 0x7F) + 64;
        bits = (sign << 31) | (exponent << 23) | (mantissa << 7);
    }

    return makef(bits);
}

void GLASS_math_packIntVector(const u32* in, u32* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(out);

    *out = in[0] & 0xFF;
    *out |= (in[1] & 0xFF) << 8;
    *out |= (in[2] & 0xFF) << 16;
    *out |= (in[3] & 0xFF) << 24;
}

void GLASS_math_unpackIntVector(u32 in, u32* out) {
    KYGX_ASSERT(out);

    out[0] = in & 0xFF;
    out[1] = (in >> 8) & 0xFF;
    out[2] = (in >> 16) & 0xFF;
    out[3] = (in >> 24) & 0xFF;
}

void GLASS_math_packFloatVector(const float* in, u32* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(out);

    const u32 cvtX = GLASS_math_f32tof24(in[0]);
    const u32 cvtY = GLASS_math_f32tof24(in[1]);
    const u32 cvtZ = GLASS_math_f32tof24(in[2]);
    const u32 cvtW = GLASS_math_f32tof24(in[3]);
    out[0] = (cvtW << 8) | (cvtZ >> 16);
    out[1] = (cvtZ << 16) | (cvtY >> 8);
    out[2] = (cvtY << 24) | cvtX;
}

void GLASS_math_unpackFloatVector(const u32* in, float* out) {
    KYGX_ASSERT(in);
    KYGX_ASSERT(out);

    out[0] = GLASS_math_f24tof32(in[2] & 0xFFFFFF);
    out[1] = GLASS_math_f24tof32((in[2] >> 24) | ((in[1] & 0xFFFF) << 8));
    out[2] = GLASS_math_f24tof32((in[1] >> 16) | ((in[0] & 0xFF) << 16));
    out[3] = GLASS_math_f24tof32(in[0] >> 8);
}