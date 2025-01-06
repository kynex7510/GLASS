#include <GLASS.h>
#include <string.h> // memcpy

#define STBI_MALLOC(sz) glassLinearAlloc(sz)
#define STBI_FREE(p) glassLinearFree(p)
#define STBI_REALLOC(p, newsz) stbiLinearRealloc(p, newsz)

static void* stbiLinearRealloc(void* p, size_t newSize) {
    void* r = glassLinearAlloc(newSize);
    if (r && p) {
        memcpy(r, p, glassLinearSize(p));
        glassLinearFree(p);
    }
    return r;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"