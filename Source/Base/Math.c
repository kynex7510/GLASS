#include "Base/Math.h"
#include "Platform/Utility.h"

static uint32_t GLASS_fbits(float f) {
    union {
        float val;
        uint32_t bits;
    } cast;

    cast.val = f;
    return cast.bits;
}

// Adapted from libctru.
// https://github.com/devkitPro/libctru/blob/master/libctru/source/gpu/gpu.c

// BEGIN CTRU CODE

static float GLASS_makef(uint32_t bits) {
    union {
        float val;
        uint32_t bits;
    } cast;

    cast.bits = bits;
    return cast.val;
}

uint32_t GLASS_math_f32tof31(float f) {
    const uint32_t bits = GLASS_fbits(f);
	const uint32_t mantissa = (bits << 9) >> 9;
	const uint32_t sign = bits >> 31;
    const int32_t exponent = ((bits << 1) >> 24) - 127 + 63;

	if (exponent < 0) {
		return sign << 30;
	}
	else if (exponent > 0x7F) {
		return sign << 30 | 0x7F << 23;
	}

	return sign << 30 | exponent << 23 | mantissa;
}

uint32_t GLASS_math_f32tof24(float f) {
    const uint32_t bits = GLASS_fbits(f);
    const uint32_t mantissa = ((bits << 9) >> 9) >> 7;
	const uint32_t sign = bits >> 31;
	const int32_t exponent = ((bits << 1) >> 24) - 127 + 63;

	if (exponent < 0) {
		return sign << 23;
	}
	else if (exponent > 0x7F) {
		return sign << 23 | 0x7F << 16;
	}

	return sign << 23 | exponent << 16 | mantissa;
}

// END CTRU CODE

uint32_t GLASS_math_f32tofixed13(float f) {
    f = GLASS_CLAMP(-15.255, 15.255, f);
    uint32_t sign = 0;

    if (f < 0.0f) {
        f = -f;
        sign |= (1u << 12);
    }

    const uint32_t i = ((uint32_t)(f) & 0xF);
    return (sign | (i << 8) | ((uint32_t)((f - i) * 1000.0f) & 0xFF));
}

float GLASS_math_f24tof32(uint32_t f) {
    const uint32_t sign = f >> 23;
    uint32_t bits = 0;

    if (!(f & 0x7FFFFF)) {
        bits = (sign << 31);
    } else if (((f >> 16) & 0xFF) == 0x7F) {
        bits = (sign << 31) | (0xFF << 23);
    } else {
        const uint32_t mantissa = f & 0xFFFF;
        const int32_t exponent = ((f >> 16) & 0x7F) + 64;
        bits = (sign << 31) | (exponent << 23) | (mantissa << 7);
    }

    return GLASS_makef(bits);
}

void GLASS_math_packIntVector(const uint32_t* in, uint32_t* out) {
    ASSERT(in);
    ASSERT(out);

    *out = in[0] & 0xFF;
    *out |= (in[1] & 0xFF) << 8;
    *out |= (in[2] & 0xFF) << 16;
    *out |= (in[3] & 0xFF) << 24;
}

void GLASS_math_unpackIntVector(uint32_t in, uint32_t* out) {
    ASSERT(out);

    out[0] = in & 0xFF;
    out[1] = (in >> 8) & 0xFF;
    out[2] = (in >> 16) & 0xFF;
    out[3] = (in >> 24) & 0xFF;
}

void GLASS_math_packFloatVector(const float* in, uint32_t* out) {
    ASSERT(in);
    ASSERT(out);

    const uint32_t cvtX = GLASS_math_f32tof24(in[0]);
    const uint32_t cvtY = GLASS_math_f32tof24(in[1]);
    const uint32_t cvtZ = GLASS_math_f32tof24(in[2]);
    const uint32_t cvtW = GLASS_math_f32tof24(in[3]);
    out[0] = (cvtW << 8) | (cvtZ >> 16);
    out[1] = (cvtZ << 16) | (cvtY >> 8);
    out[2] = (cvtY << 24) | cvtX;
}

void GLASS_math_unpackFloatVector(const uint32_t* in, float* out) {
    ASSERT(in);
    ASSERT(out);

    out[0] = GLASS_math_f24tof32(in[2] & 0xFFFFFF);
    out[1] = GLASS_math_f24tof32((in[2] >> 24) | ((in[1] & 0xFFFF) << 8));
    out[2] = GLASS_math_f24tof32((in[1] >> 16) | ((in[0] & 0xFF) << 16));
    out[3] = GLASS_math_f24tof32(in[0] >> 8);
}