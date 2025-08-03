#include <GLASS.h>
#include <string.h> // memcpy

#define STBI_MALLOC(sz) glassLinearAlloc((sz))
#define STBI_FREE(p) glassLinearFree((p))
#define STBI_REALLOC(p,newsz) glassLinearRealloc((p), (newsz))

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"