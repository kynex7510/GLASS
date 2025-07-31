#include <GLASS.h>
#include <string.h> // memcpy

#define STBI_MALLOC(sz) glassLinearAlloc((sz))
#define STBI_FREE(p) glassLinearFree((p))
#define STBI_REALLOC(p,newsz) stbiLinearRealloc((p), (newsz))

void* stbiLinearRealloc(void* p, size_t size) {
    if (!p)
        return STBI_MALLOC(size);

    void* q = STBI_MALLOC(size);
    if (q) {
        const size_t oldSize = glassLinearSize(p);
        memcpy(q, p, size < oldSize ? size : oldSize);
        STBI_FREE(p);
    }

    return q;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"