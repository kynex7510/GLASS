#define GLASS_PLATFORM_SOURCE
#include "Platform/Includes.h"
#include "Platform/Utility.h"

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

bool GLASS_utility_flushCache(const void* addr, size_t size) {
    if (glassIsLinear(addr))
        return R_SUCCEEDED(GSPGPU_FlushDataCache(addr, size));

    return true;
}

bool GLASS_utility_invalidateCache(const void* addr, size_t size) {
    if (glassIsLinear(addr) || glassIsVRAM(addr))
        return R_SUCCEEDED(GSPGPU_InvalidateDataCache(addr, size));

    return true;
}

uint32_t GLASS_utility_convertVirtToPhys(const void* addr) { return osConvertVirtToPhys(addr); }

void* GLASS_utility_convertPhysToVirt(uint32_t addr) {
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