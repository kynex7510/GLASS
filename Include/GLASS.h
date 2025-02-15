#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Tex3DS.h>

#define GLASS_VERSION_2_0 0x20 // OpenGL ES 2.0

typedef struct glassCtxImpl* glassCtx;
typedef u32 glassVersion;

// Target screen.
typedef enum {
#if defined(GLASS_BAREMETAL)
    GLASS_SCREEN_TOP = GFX_LCD_TOP,
    GLASS_SCREEN_BOTTOM = GFX_LCD_BOT,
#else
    GLASS_SCREEN_TOP = GFX_TOP,
    GLASS_SCREEN_BOTTOM = GFX_BOTTOM,
#endif // GLASS_BAREMETAL
} glassScreen;

// Target screen side.
typedef enum {
#if defined(GLASS_BAREMETAL)
    GLASS_SIDE_LEFT = GFX_SIDE_LEFT,
    GLASS_SIDE_RIGHT = GFX_SIDE_RIGHT,
#else
    GLASS_SIDE_LEFT = GFX_LEFT,
    GLASS_SIDE_RIGHT = GFX_RIGHT,
#endif // GLASS_BAREMETAL
} glassSide;

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
} glassGpuCommandList;

// Context settings.
typedef struct {
    glassScreen targetScreen;        // Draw target screen (default: GLASS_SCREEN_TOP).
    glassSide targetSide;            // Draw target side (default: GLASS_SCREEN_LEFT).
    bool vsync;                      // Vsync (default: true).
    bool verticalFlip;               // Flip display buffer vertically (default: false).
    glassGpuCommandList gpuCmdList;  // GPU command list (default: all 0).
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