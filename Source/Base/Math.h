#ifndef _GLASS_BASE_MATH_H
#define _GLASS_BASE_MATH_H

#include "Base/Types.h"

#define GLASS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define GLASS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define GLASS_CLAMP(min, max, val) (GLASS_MAX((min), GLASS_MIN((max), (val))))

u32 GLASS_math_f32tofixed13(float f);
u32 GLASS_math_f32tof31(float f);
u32 GLASS_math_f32tof24(float f);
float GLASS_math_f24tof32(u32 f);

void GLASS_math_packIntVector(const u32* in, u32* out);
void GLASS_math_unpackIntVector(u32 in, u32* out);

void GLASS_math_packFloatVector(const float* in, u32* out);
void GLASS_math_unpackFloatVector(const u32* in, float* out);

#endif /* _GLASS_BASE_MATH_H */