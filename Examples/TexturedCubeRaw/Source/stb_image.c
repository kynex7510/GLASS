#include <3ds.h>
#include <string.h> // memcpy

#define STBI_MALLOC(sz) linearAlloc(sz)
#define STBI_FREE(p) linearFree(p)
#define STBI_REALLOC(p, newsz) stbiLinearRealloc(p, newsz)

static void* stbiLinearRealloc(void* p, size_t newSize) {
    void* r = linearAlloc(newSize);
    if (r && p) {
        memcpy(r, p, linearGetSize(p));
        linearFree(p);
    }
    return r;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"