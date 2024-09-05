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

void* GLASS_linearAllocDefault(size_t size) {
    void* p = linearAlloc(size);
    if (p)
        memset(p, 0, size);

    return p;
}

void GLASS_linearFreeDefault(void* p) { linearFree(p); }
void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos) { return vramAllocAt(size, pos); }
void GLASS_vramFreeDefault(void* p) { vramFree(p); }

WEAK void* GLASS_virtualAlloc(size_t size) { return GLASS_virtualAllocDefault(size); }
WEAK void GLASS_virtualFree(void* p) { GLASS_virtualFreeDefault(p); }
WEAK void* GLASS_linearAlloc(size_t size) { return GLASS_linearAllocDefault(size); }
WEAK void GLASS_linearFree(void* p) { GLASS_linearFreeDefault(p); }
WEAK void* GLASS_vramAlloc(size_t size, vramAllocPos pos) { return GLASS_vramAllocDefault(size, pos); }
WEAK void GLASS_vramFree(void* p) { GLASS_vramFreeDefault(p); }