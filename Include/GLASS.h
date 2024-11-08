#ifndef _GLASS_H
#define _GLASS_H

#include <3ds.h>

#define GLASS_VERSION_2_0 0x20 // OpenGL ES 2.0

typedef void* glassCtx;
typedef u32 glassVersion;

// Context initialization parameters.
typedef struct {
    u8 version;             // Context version.
    bool flushAllLinearMem; // Whether to flush all linear memory (default: true).
} glassInitParams;

// Context settings.
typedef struct {
    gfxScreen_t targetScreen;        // Draw target screen (default: GFX_TOP).
    gfx3dSide_t targetSide;          // Draw target side (default: GFX_LEFT).
    GX_TRANSFER_SCALE transferScale; // Anti-aliasing (default: GX_TRANSFER_SCALE_NO).
} glassSettings;

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

glassCtx glassCreateContext(glassVersion version);
glassCtx glassCreateContextEx(const glassInitParams* initParams, const glassSettings* settings);
void glassDestroyContext(glassCtx ctx);
void glassBindContext(glassCtx ctx);

void glassReadSettings(glassCtx ctx, glassSettings* settings);
void glassWriteSettings(glassCtx ctx, const glassSettings* settings);

void glassSwapBuffers(void);

void* glassVirtualAlloc(size_t size);
void glassVirtualFree(void* p);
size_t glassVirtualSize(void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(void* p);

void* glassVRAMAlloc(size_t size, vramAllocPos pos);
void glassVRAMFree(void* p);
size_t glassVRAMSize(void* p);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_H */