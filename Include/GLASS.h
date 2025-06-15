#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Tex3DS.h>

#define GLASS_VERSION_2_0 0x20 // OpenGL ES 2.0

typedef struct glassCtxImpl* glassCtx;
typedef u32 glassVersion;

// Context initialization parameters.
typedef struct {
    u8 version;             // Context version.
    bool flushAllLinearMem; // Whether to flush all linear memory (default: true).
} glassInitParams;

// GPU command list.
typedef struct {
    void* mainBuffer;    // Main command buffer.
    void* secondBuffer;  // Second command buffer.
    size_t capacity;     // Max size of each buffer, in bytes.
    size_t offset;       // Offset of the current GPU command location.
} glassGPUCommandList;

// Context settings.
typedef struct {
    gfxScreen_t targetScreen;        // Draw target screen (default: GFX_TOP).
    gfx3dSide_t targetSide;          // Draw target side (default: GFX_LEFT).
    bool verticalFlip;               // Flip display buffer vertically (default: false).
    glassGPUCommandList gpuCmdList;  // GPU command list (default: all 0).
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
void glassSwapContextBuffers(glassCtx top, glassCtx bottom);

void* glassVirtualAlloc(size_t size);
void glassVirtualFree(void* p);
size_t glassVirtualSize(const void* p);
bool glassIsVirtual(const void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(const void* p);
bool glassIsLinear(const void* p);

void* glassVRAMAlloc(size_t size, vramAllocPos pos);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_H */