#include "Base/Utility.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memset
#include <malloc.h> // malloc_usable_size

void* GLASS_virtualAllocDefault(size_t size) {
    void* p = malloc(size);
    if (p)
        memset(p, 0, size);

    return p;
}

void GLASS_virtualFreeDefault(void* p) { free(p); }
size_t GLASS_virtualSizeDefault(const void* p) { return malloc_usable_size((void*)p); }

bool GLASS_isVirtualDefault(const void* p) {
#if defined(GLASS_BAREMETAL)
    return !glassIsVRAM(p);
#else
    return !(glassIsLinear(p) && glassIsVRAM(p));
#endif // GLASS_BAREMETAL
}

void* GLASS_linearAllocDefault(size_t size) {
#if defined(GLASS_BAREMETAL)
    return GLASS_virtualAllocDefault(size);
#else
    void* p = linearAlloc(size);
    if (p)
        memset(p, 0, size);

    return p;
#endif // GLASS_BAREMETAL
}

void GLASS_linearFreeDefault(void* p) {
#if defined(GLASS_BAREMETAL)
    GLASS_virtualFreeDefault(p);
#else
    linearFree(p);
#endif // GLASS_BAREMETAL
}

size_t GLASS_linearSizeDefault(const void* p) {
#if defined(GLASS_BAREMETAL)
    return GLASS_virtualSizeDefault(p);
#else
    return linearGetSize((void*)p);
#endif // GLASS_BAREMETAL
}

bool GLASS_isLinearDefault(const void* p) {
#if defined(GLASS_BAREMETAL)
    return GLASS_isVirtualDefault(p);
#else
    const u32 addr = (u32)p;
    return ((addr >= OS_FCRAM_VADDR) && (addr < (OS_FCRAM_VADDR + OS_FCRAM_SIZE)));
#endif // GLASS_BAREMETAL
}

void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos) { return vramAllocAt(size, pos); }
void GLASS_vramFreeDefault(void* p) { vramFree(p); }
size_t GLASS_vramSizeDefault(const void* p) { return vramGetSize((void*)p); }

bool GLASS_isVramDefault(const void* p) {
    const u32 addr = (u32)p;

#if defined(GLASS_BAREMETAL)
    return ((addr >= VRAM_BASE) && (addr < (VRAM_BASE + VRAM_SIZE)));
#else
    return ((addr >= OS_VRAM_VADDR) && (addr < (OS_VRAM_VADDR + OS_VRAM_SIZE)));
#endif // GLASS_BAREMETAL
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