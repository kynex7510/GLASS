#include "GLASS.h"

#include <stddef.h>
#include <stdio.h>

static size_t g_VirtualMem = 0;
static size_t g_LinearMem = 0;
static size_t g_VRAM = 0;

static size_t g_VirtualAllocs = 0;
static size_t g_LinearAllocs = 0;
static size_t g_VRAMAllocs = 0;

static bool g_NeedsInit = true;

extern void* GLASS_virtualAllocDefault(size_t size);
extern void GLASS_virtualFreeDefault(void* p);
extern size_t GLASS_virtualSizeDefault(void* p);
extern void* GLASS_linearAllocDefault(size_t size);
extern void GLASS_linearFreeDefault(void* p);
extern size_t GLASS_linearSizeDefault(void* p);
extern void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos);
extern void GLASS_vramFreeDefault(void* p);
extern size_t GLASS_vramSizeDefault(void* p);

static void printData(size_t size, size_t allocs) {
    if (size > 1 * 1000 * 1000) {
        printf("%.3f MB", size / (1.0f * 1000 * 1000));
    } else if (size > 1 * 1000) {
        printf("%.3f KB", size / (1.0f * 1000));
    } else {
        printf("%u bytes", size);
    }

    printf(" (%u %s)\n", allocs, allocs == 1 ? "allocation" : "allocations");
}

void refreshDebugStats(void) {
    if (g_NeedsInit) {
        consoleInit(GFX_BOTTOM, NULL);
        g_NeedsInit = false;
    }

    consoleClear();
    printf("MEMORY USAGE\n");

    printf("- Heap: ");
    printData(g_VirtualMem + g_LinearMem, g_VirtualAllocs + g_LinearAllocs);

    printf("-- Virtual: ");
    printData(g_VirtualMem, g_VirtualAllocs);

    printf("-- Linear: ");
    printData(g_LinearMem, g_LinearAllocs);

    printf("- VRAM: ");
    printData(g_VRAM, g_VRAMAllocs);

    printf("- Total: ");
    printData(g_VirtualMem + g_LinearMem + g_VRAM, g_VirtualAllocs + g_LinearAllocs + g_VRAMAllocs);
}

void* glassVirtualAlloc(size_t size) {
    void* p = GLASS_virtualAllocDefault(size);
    if (p) {
        g_VirtualMem += size;
        ++g_VirtualAllocs;
        refreshDebugStats();
    }

    return p;
}

void glassVirtualFree(void* p) {
    g_VirtualMem -= GLASS_virtualSizeDefault(p);
    --g_VirtualAllocs;
    refreshDebugStats();
    GLASS_virtualFreeDefault(p);
}

void* glassLinearAlloc(size_t size) {
    void* p = GLASS_linearAllocDefault(size);
    if (p) {
        g_LinearMem += size;
        ++g_LinearAllocs;
        refreshDebugStats();
    }

    return p;
}

void glassLinearFree(void* p) {
    g_LinearMem -= GLASS_linearSizeDefault(p);
    --g_LinearAllocs;
    refreshDebugStats();
    GLASS_linearFreeDefault(p);
}

void* glassVRAMAlloc(size_t size, vramAllocPos pos) {
    void* p = GLASS_vramAllocDefault(size, pos);
    if (p) {
        g_VRAM += size;
        ++g_VRAMAllocs;
        refreshDebugStats();
    }

    return p;
}

void glassVRAMFree(void* p) {
    g_VRAM -= GLASS_vramSizeDefault(p);
    --g_VRAMAllocs;
    refreshDebugStats();
    GLASS_vramFreeDefault(p);
}