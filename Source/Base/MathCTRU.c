// Adapted from https://github.com/devkitPro/libctru/blob/master/libctru/source/gpu/gpu.c

/**
 * Copyright (c) 2014-2025 libctru contributors
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "Base/Math.h"

static inline u32 fbits(float f) {
    union {
        float val;
        u32 bits;
    } cast;

    cast.val = f;
    return cast.bits;
}

u32 GLASS_math_f32tof31(float f) {
    const u32 bits = fbits(f);
    const u32 mantissa = (bits << 9) >> 9;
    const u32 sign = bits >> 31;
    const s32 exponent = ((bits << 1) >> 24) - 127 + 63;

    if (exponent < 0) {
        return sign << 30;
    }
    else if (exponent > 0x7F) {
        return sign << 30 | 0x7F << 23;
    }

    return sign << 30 | exponent << 23 | mantissa;
}

u32 GLASS_math_f32tof24(float f) {
    const u32 bits = fbits(f);
    const u32 mantissa = ((bits << 9) >> 9) >> 7;
    const u32 sign = bits >> 31;
    const s32 exponent = ((bits << 1) >> 24) - 127 + 63;

    if (exponent < 0) {
        return sign << 23;
    }
    else if (exponent > 0x7F) {
        return sign << 23 | 0x7F << 16;
    }

    return sign << 23 | exponent << 16 | mantissa;
}