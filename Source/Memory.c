#include "Utility.h"

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
size_t GLASS_virtualSizeDefault(void* p) { return malloc_usable_size(p); }

void* GLASS_linearAllocDefault(size_t size) {
    void* p = linearAlloc(size);
    if (p)
        memset(p, 0, size);

    return p;
}

void GLASS_linearFreeDefault(void* p) { linearFree(p); }
size_t GLASS_linearSizeDefault(void* p) { return linearGetSize(p); }
void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos) { return vramAllocAt(size, pos); }
void GLASS_vramFreeDefault(void* p) { vramFree(p); }
size_t GLASS_vramSizeDefault(void* p) { return vramGetSize(p); }

WEAK void* glassVirtualAlloc(size_t size) { return GLASS_virtualAllocDefault(size); }
WEAK void glassVirtualFree(void* p) { GLASS_virtualFreeDefault(p); }
WEAK size_t glassVirtualSize(void* p) { return GLASS_virtualSizeDefault(p); }
WEAK void* glassLinearAlloc(size_t size) { return GLASS_linearAllocDefault(size); }
WEAK void glassLinearFree(void* p) { GLASS_linearFreeDefault(p); }
WEAK size_t glassLinearSize(void* p) { return GLASS_linearSizeDefault(p); }
WEAK void* glassVRAMAlloc(size_t size, vramAllocPos pos) { return GLASS_vramAllocDefault(size, pos); }
WEAK void glassVRAMFree(void* p) { GLASS_vramFreeDefault(p); }
WEAK size_t glassVRAMSize(void* p) { return GLASS_vramSizeDefault(p); }