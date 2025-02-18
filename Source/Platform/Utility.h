#ifndef _GLASS_PLATFORM_UTILITY_H
#define _GLASS_PLATFORM_UTILITY_H

#include "Base/Types.h"

#define GLASS_STRINGIFY(x) #x
#define GLASS_AS_STRING(x) GLASS_STRINGIFY(x)

#ifndef NDEBUG

#define LOG(msg) GLASS_utility_logImpl((msg))

#define ASSERT(cond)                                                                                        \
    do {                                                                                                    \
        if (!(cond)) {                                                                                      \
            LOG("Assertion failed: " #cond "\nIn file: " __FILE__ "\nOn line: " GLASS_AS_STRING(__LINE__)); \
            GLASS_utility_abort();                                                                          \
        }                                                                                                   \
    } while (false)

#define UNREACHABLE(msg)                                                                               \
    do {                                                                                               \
        LOG("Unreachable point: " msg "\nIn file: " __FILE__ "\nOn line: " GLASS_AS_STRING(__LINE__)); \
        GLASS_utility_abort();                                                                         \
    } while (false)

void GLASS_utility_logImpl(const char* msg);

#else

#define LOG(msg)
#define ASSERT(cond) (void)(cond)
#define UNREACHABLE(msg) GLASS_utility_abort()

#endif // NDEBUG

void GLASS_utility_abort(void) __attribute__((noreturn));

bool GLASS_utility_flushCache(const void* addr, size_t size);
bool GLASS_utility_invalidateCache(const void* addr, size_t size);

uint32_t GLASS_utility_convertVirtToPhys(const void* addr);
void* GLASS_utility_convertPhysToVirt(uint32_t addr);

#endif /* _GLASS_PLATFORM_UTILITY_H */