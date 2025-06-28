#include <GLASS.h>

#define WEAK  __attribute__((weak))

WEAK void* glassHeapAlloc(size_t size) { return kygxAlloc(KYGX_MEM_HEAP, size); }
WEAK void glassHeapFree(void* p) { kygxFree(p); }
WEAK size_t glassHeapSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsHeap(const void* p) { return kygxIsHeap(p); }

WEAK void* glassLinearAlloc(size_t size) { return kygxAlloc(KYGX_MEM_LINEAR, size); }
WEAK void glassLinearFree(void* p) { kygxFree(p); }
WEAK size_t glassLinearSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsLinear(const void* p) { return kygxIsLinear(p); }

WEAK void* glassVRAMAlloc(KYGXVRAMBank bank, size_t size) { return kygxAllocVRAM(bank, size); }
WEAK void glassVRAMFree(void* p) { kygxFree(p); }
WEAK size_t glassVRAMSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsVRAM(const void* p) { return kygxIsVRAM(p); }