#ifndef _GLASS_BASE_MATH_H
#define _GLASS_BASE_MATH_H

#include "Base/Types.h"
#include "Platform/Utility.h"

#define GLASS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define GLASS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define GLASS_CLAMP(min, max, val) (GLASS_MAX((min), GLASS_MIN((max), (val))))

inline bool GLASS_math_isPowerOf2(uint32_t v) { return !(v & (v - 1)); }

inline bool GLASS_math_isAligned(size_t v, size_t alignment) {
    ASSERT(GLASS_math_isPowerOf2(alignment));
    return !(v & (alignment - 1));
}

inline size_t GLASS_math_alignDown(size_t v, size_t alignment) {
    ASSERT(GLASS_math_isPowerOf2(alignment));
    return v & ~(alignment - 1);
}

inline size_t GLASS_math_alignUp(size_t v, size_t alignment) {
    ASSERT(GLASS_math_isPowerOf2(alignment));
    return (v + alignment) & ~(alignment - 1);
}

uint32_t GLASS_math_f32tofixed13(float f);
uint32_t GLASS_math_f32tof31(float f);
uint32_t GLASS_math_f32tof24(float f);
float GLASS_math_f24tof32(uint32_t f);

void GLASS_math_packIntVector(const uint32_t* in, uint32_t* out);
void GLASS_math_unpackIntVector(uint32_t in, uint32_t* out);

void GLASS_math_packFloatVector(const float* in, uint32_t* out);
void GLASS_math_unpackFloatVector(const uint32_t* in, float* out);

#endif /* _GLASS_BASE_MATH_H */