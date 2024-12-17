#include "Base/Utility.h"

#include <stdlib.h> // abort

#ifndef NDEBUG

#include <stdio.h> // file utilities
#include <string.h> // strlen

static FILE* g_LogFile = NULL;

void GLASS_utility_logImpl(const char* msg) {
    if (g_LogFile == NULL)
        g_LogFile = fopen("sdmc:/GLASS.log", "w");

    if (g_LogFile)
        fwrite(msg, strlen(msg), 1, g_LogFile);
}

void GLASS_utility_abort(void) {
    if (g_LogFile) {
        fflush(g_LogFile);
        fclose(g_LogFile);
        g_LogFile = NULL;
    }

    abort();
    __builtin_unreachable();
}

#else

void GLASS_utility_abort(void) {
    abort();
    __builtin_unreachable();
}

#endif // NDEBUG

bool GLASS_utility_isPowerOf2(u32 v) { return !(v & (v - 1)); }
u32 GLASS_utility_nextPowerOf2(u32 v) { return 1 << (32 - __builtin_clz(v)); }

void* GLASS_utility_convertPhysToVirt(u32 addr) {
#define CONVERT_REGION(_name)                                              \
    if (addr >= OS_##_name##_PADDR &&                                      \
        addr < (OS_##_name##_PADDR + OS_##_name##_SIZE))                   \
        return (void*)(addr - (OS_##_name##_PADDR + OS_##_name##_VADDR));

    CONVERT_REGION(FCRAM);
    CONVERT_REGION(VRAM);
    CONVERT_REGION(OLD_FCRAM);
    CONVERT_REGION(DSPRAM);
    CONVERT_REGION(QTMRAM);
    CONVERT_REGION(MMIO);

#undef CONVERT_REGION
    return NULL;
}

float GLASS_utility_f24tof32(u32 f) {
    union {
        float val;
        u32 bits;
    } cast;

    const u32 sign = f >> 23;

    if (!(f & 0x7FFFFF)) {
        cast.bits = (sign << 31);
    } else if (((f >> 16) & 0xFF) == 0x7F) {
        cast.bits = (sign << 31) | (0xFF << 23);
    } else {
        const u32 mantissa = f & 0xFFFF;
        const s32 exponent = ((f >> 16) & 0x7F) + 64;
        cast.bits = (sign << 31) | (exponent << 23) | (mantissa << 7);
    }

    return cast.val;
}

u32 GLASS_utility_f32tofixed13(float f) {
    f = CLAMP(-15.255, 15.255, f);
    u32 sign = 0;

    if (f < 0.0f) {
        f = -f;
        sign |= (1u << 12);
    }

    const u32 i = ((u32)(f) & 0xF);
    return (sign | (i << 8) | ((u32)((f - i) * 1000.0f) & 0xFF));
}

void GLASS_utility_packIntVector(const u32* in, u32* out) {
    ASSERT(in);
    ASSERT(out);

    *out = in[0] & 0xFF;
    *out |= (in[1] & 0xFF) << 8;
    *out |= (in[2] & 0xFF) << 16;
    *out |= (in[3] & 0xFF) << 24;
}

void GLASS_utility_unpackIntVector(u32 in, u32* out) {
    ASSERT(out);

    out[0] = in & 0xFF;
    out[1] = (in >> 8) & 0xFF;
    out[2] = (in >> 16) & 0xFF;
    out[3] = (in >> 24) & 0xFF;
}

void GLASS_utility_packFloatVector(const float* in, u32* out) {
    ASSERT(in);
    ASSERT(out);

    const u32 cvtX = f32tof24(in[0]);
    const u32 cvtY = f32tof24(in[1]);
    const u32 cvtZ = f32tof24(in[2]);
    const u32 cvtW = f32tof24(in[3]);
    out[0] = (cvtW << 8) | (cvtZ >> 16);
    out[1] = (cvtZ << 16) | (cvtY >> 8);
    out[2] = (cvtY << 24) | cvtX;
}

void GLASS_utility_unpackFloatVector(const u32* in, float* out) {
    ASSERT(in);
    ASSERT(out);

    out[0] = GLASS_utility_f24tof32(in[2] & 0xFFFFFF);
    out[1] = GLASS_utility_f24tof32((in[2] >> 24) | ((in[1] & 0xFFFF) << 8));
    out[2] = GLASS_utility_f24tof32((in[1] >> 16) | ((in[0] & 0xFF) << 16));
    out[3] = GLASS_utility_f24tof32(in[0] >> 8);
}

size_t GLASS_utility_getRenderbufferBpp(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 32;
        case GL_RGB8_OES:
        case GL_DEPTH_COMPONENT24_OES:
            return 24;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
        case GL_DEPTH_COMPONENT16:
            return 16;
    }

    UNREACHABLE("Invalid format!");
}