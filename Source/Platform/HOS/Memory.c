#define GLASS_PLATFORM_SOURCE
#include "Platform/Includes.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memset
#include <malloc.h> // malloc_usable_size

#define WEAK __attribute__((weak))

void* GLASS_virtualAllocDefault(size_t size) {
    void* p = malloc(size);
    if (p)
        memset(p, 0, size);

    return p;
}

void GLASS_virtualFreeDefault(void* p) { free(p); }
size_t GLASS_virtualSizeDefault(const void* p) { return malloc_usable_size((void*)p); }
bool GLASS_isVirtualDefault(const void* p) { return !(glassIsLinear(p) && glassIsVRAM(p)); }

void* GLASS_linearAllocDefault(size_t size) {
    void* p = linearAlloc(size);
    if (p)
        memset(p, 0, size);

    return p;
}

void GLASS_linearFreeDefault(void* p) { linearFree(p); }
size_t GLASS_linearSizeDefault(const void* p) { return linearGetSize((void*)p); }

bool GLASS_isLinearDefault(const void* p) {
    const uint32_t addr = (uint32_t)p;
    return ((addr >= OS_FCRAM_VADDR) && (addr < (OS_FCRAM_VADDR + OS_FCRAM_SIZE)));
}

void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos) { return vramAllocAt(size, pos); }
void GLASS_vramFreeDefault(void* p) { vramFree(p); }
size_t GLASS_vramSizeDefault(const void* p) { return vramGetSize((void*)p); }

bool GLASS_isVramDefault(const void* p) {
    const uint32_t addr = (uint32_t)p;
    return ((addr >= OS_VRAM_VADDR) && (addr < (OS_VRAM_VADDR + OS_VRAM_SIZE)));
}

WEAK void* glassVirtualAlloc(size_t size) { return GLASS_virtualAllocDefault(size); }
WEAK void glassVirtualFree(void* p) { GLASS_virtualFreeDefault(p); }
WEAK size_t glassVirtualSize(const void* p) { return GLASS_virtualSizeDefault(p); }
WEAK bool glassIsVirtual(const void* p) { return GLASS_isVirtualDefault(p); }

WEAK void* glassLinearAlloc(size_t size) { return GLASS_linearAllocDefault(size); }
WEAK void glassLinearFree(void* p) { GLASS_linearFreeDefault(p); }
WEAK size_t glassLinearSize(const void* p) { return GLASS_linearSizeDefault(p); }
WEAK bool glassIsLinear(const void* p) { return GLASS_isLinearDefault(p); }

WEAK void* glassVRAMAlloc(size_t size, vramAllocPos pos) { return GLASS_vramAllocDefault(size, pos); }
WEAK void glassVRAMFree(void* p) { GLASS_vramFreeDefault(p); }
WEAK size_t glassVRAMSize(const void* p) { return GLASS_vramSizeDefault(p); }
WEAK bool glassIsVRAM(const void* p) { return GLASS_isVramDefault(p); }