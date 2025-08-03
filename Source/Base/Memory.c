/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <GLASS.h>

#define WEAK  __attribute__((weak))

WEAK void* glassHeapAlloc(size_t size) {
    void* p = kygxAlloc(KYGX_MEM_HEAP, size);
    if (p)
        memset(p, 0, size);

    return p;
}

WEAK void glassHeapFree(void* p) { kygxFree(p); }
WEAK void* glassHeapRealloc(void* p, size_t newSize) { return kygxRealloc(p, newSize); }
WEAK size_t glassHeapSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsHeap(const void* p) { return kygxIsHeap(p); }

WEAK void* glassLinearAlloc(size_t size) { return kygxAlloc(KYGX_MEM_LINEAR, size); }
WEAK void glassLinearFree(void* p) { kygxFree(p); }
WEAK void* glassLinearRealloc(void* p, size_t newSize) { return kygxRealloc(p, newSize); }
WEAK size_t glassLinearSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsLinear(const void* p) { return kygxIsLinear(p); }

WEAK void* glassVRAMAlloc(size_t size, KYGXVRAMBank bank) { return kygxAllocVRAM(bank, size); }
WEAK void glassVRAMFree(void* p) { kygxFree(p); }
WEAK void* glassVRAMRealloc(void* p, size_t newSize) { kygxRealloc(p, newSize); }
WEAK size_t glassVRAMSize(const void* p) { return kygxGetAllocSize(p); }
WEAK bool glassIsVRAM(const void* p) { return kygxIsVRAM(p); }