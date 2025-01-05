#ifndef _GLASS_BASE_UTILITY_H
#define _GLASS_BASE_UTILITY_H

#include "Types.h"

#define STRINGIFY(x) #x
#define AS_STRING(x) STRINGIFY(x)

#define NORETURN __attribute__((noreturn))
#define WEAK __attribute__((weak))

#ifndef NDEBUG

#define LOG(msg) GLASS_utility_logImpl((msg))

#define ASSERT(cond)                                                                                  \
    do {                                                                                              \
        if (!(cond)) {                                                                                \
            LOG("Assertion failed: " #cond "\nIn file: " __FILE__ "\nOn line: " AS_STRING(__LINE__)); \
            GLASS_utility_abort();                                                                    \
        }                                                                                             \
    } while (false)

#define UNREACHABLE(msg)                                                                         \
    do {                                                                                         \
        LOG("Unreachable point: " msg "\nIn file: " __FILE__ "\nOn line: " AS_STRING(__LINE__)); \
        GLASS_utility_abort();                                                                   \
    } while (false)

void GLASS_utility_logImpl(const char* msg);

#else

#define LOG(msg)
#define ASSERT(cond) (void)(cond)
#define UNREACHABLE(msg) GLASS_utility_abort()

#endif // NDEBUG

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, max, val) (MAX((min), MIN((max), (val))))

void GLASS_utility_abort(void) NORETURN;

bool GLASS_utility_isPowerOf2(u32 v);
bool GLASS_utility_isAligned(size_t v, size_t alignment);
size_t GLASS_utility_alignDown(size_t v, size_t alignment);
size_t GLASS_utility_alignUp(size_t v, size_t alignment);

bool GLASS_utility_flushCache(const void* addr, size_t size);
bool GLASS_utility_invalidateCache(const void* addr, size_t size);

void* GLASS_utility_convertPhysToVirt(u32 addr);
float GLASS_utility_f24tof32(u32 f);
u32 GLASS_utility_f32tofixed13(float f);

void GLASS_utility_packIntVector(const u32* in, u32* out);
void GLASS_utility_unpackIntVector(u32 in, u32* out);

void GLASS_utility_packFloatVector(const float* in, u32* out);
void GLASS_utility_unpackFloatVector(const u32* in, float* out);

#endif /* _GLASS_BASE_UTILITY_H */