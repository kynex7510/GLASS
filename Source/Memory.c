#include "Memory.h"
#include "Utility.h"

#include <malloc.h>

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

WEAK void* GLASS_virtualAlloc(size_t size) { return GLASS_virtualAllocDefault(size); }
WEAK void GLASS_virtualFree(void* p) { GLASS_virtualFreeDefault(p); }
WEAK size_t GLASS_virtualSize(void* p) { return GLASS_virtualSizeDefault(p); }
WEAK void* GLASS_linearAlloc(size_t size) { return GLASS_linearAllocDefault(size); }
WEAK void GLASS_linearFree(void* p) { GLASS_linearFreeDefault(p); }
WEAK size_t GLASS_linearSize(void* p) { return GLASS_linearSizeDefault(p); }
WEAK void* GLASS_vramAlloc(size_t size, vramAllocPos pos) { return GLASS_vramAllocDefault(size, pos); }
WEAK void GLASS_vramFree(void* p) { GLASS_vramFreeDefault(p); }
WEAK size_t GLASS_vramSize(void* p) { return GLASS_vramSizeDefault(p); }