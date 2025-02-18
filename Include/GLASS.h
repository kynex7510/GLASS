#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Tex3DS.h>

#define GLASS_VERSION_2_0 0x20 // OpenGL ES 2.0

typedef struct glassCtxImpl* glassCtx;
typedef uint8_t glassVersion;

// Target screen.
typedef enum {
    GLASS_SCREEN_TOP = 0,
    GLASS_SCREEN_BOTTOM = 1,
} glassScreen;

// Target screen side.
typedef enum {
    GLASS_SIDE_LEFT = 0,
    GLASS_SIDE_RIGHT = 1,
} glassSide;

// VRAM allocation position.
typedef enum {
    GLASS_VRAM_BANK_A = 0,
    GLASS_VRAM_BANK_B = 1,
} glassVRAMAllocPos;

// Context initialization parameters.
typedef struct {
    glassVersion version;   // Context version.
    bool flushAllLinearMem; // Whether to flush all linear memory (default: true).
} glassInitParams;

// GPU command list.
typedef struct {
    uint8_t* mainBuffer;   // Main command buffer.
    uint8_t* secondBuffer; // Second command buffer.
    size_t capacity;       // Max size of each buffer, in bytes.
    size_t offset;         // Offset of the current GPU command location.
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

void* glassVRAMAlloc(size_t size, glassVRAMAllocPos pos);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_H */