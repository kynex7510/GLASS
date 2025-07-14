#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Defs.h>

typedef struct GLASSCtxImpl* GLASSCtx;

/// @brief OpenGL ES version.
typedef enum {
    GLASS_VERSION_2_0, ///< OpenGL ES 2.0
} GLASSVersion;

/// @brief Target screen.
typedef enum {
    GLASS_SCREEN_TOP,    ///< Top screen.
    GLASS_SCREEN_BOTTOM, ///< Bottom screen.
} GLASSScreen;

/// @brief Target screen side.
typedef enum {
    GLASS_SIDE_LEFT,  ///< Left side.
    GLASS_SIDE_RIGHT, ///< Right side.
} GLASSSide;

/// @brief GPU command list.
typedef struct {
    void* mainBuffer;    ///< Main command buffer.
    void* secondBuffer;  ///< Second command buffer.
    size_t capacity;     ///< Max size of each buffer, in bytes.
    size_t offset;       ///< Offset of the current GPU command location.
} GLASSGPUCommandList;

/// @brief Framebuffer dimension downscale (anti-aliasing).
typedef enum {
    GLASS_DOWNSCALE_NONE, ///< No downscale.
    GLASS_DOWNSCALE_1X2,  ///< Halve height.
    GLASS_DOWNSCALE_2X2,  ///< Halve width and height.
} GLASSDownscale;

/// @brief Context parameters.
typedef struct {
    GLASSVersion version;           ///< Context version.
    GLASSScreen targetScreen;       ///< Draw target screen (default: GLASS_SCREEN_TOP).
    GLASSSide targetSide;           ///< Draw target side (default: GLASS_SIDE_LEFT).
    GLASSGPUCommandList GPUCmdList; ///< GPU command list (default: all NULL).
    bool vsync;                     ///< Enable VSync (default: true).
    bool horizontalFlip;            ///< Flip display buffer horizontally (default: false).
    bool flushAllLinearMem;         ///< Whether to flush all linear memory (default: true).
    GLASSDownscale downscale;       ///< Set downscale for anti-aliasing (default: GLASS_DOWNSCALE_NONE).
} GLASSCtxParams;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Get default context params.
KYGX_INLINE void glassGetDefaultContextParams(GLASSCtxParams* ctxParams, GLASSVersion version) {
    KYGX_ASSERT(ctxParams);

    ctxParams->version = version;
    ctxParams->targetScreen = GLASS_SCREEN_TOP;
    ctxParams->targetSide = GLASS_SIDE_LEFT;
    ctxParams->GPUCmdList.mainBuffer = NULL;
    ctxParams->GPUCmdList.secondBuffer = NULL;
    ctxParams->GPUCmdList.capacity = 0;
    ctxParams->GPUCmdList.offset = 0;
    ctxParams->vsync = true;
    ctxParams->horizontalFlip = false;
    ctxParams->flushAllLinearMem = true;
    ctxParams->downscale = GLASS_DOWNSCALE_NONE;
}

// Create context.
GLASSCtx glassCreateContext(const GLASSCtxParams* ctxParams);

// Create context with default params.
KYGX_INLINE GLASSCtx glassCreateDefaultContext(GLASSVersion version) {
    GLASSCtxParams defaultParams;
    glassGetDefaultContextParams(&defaultParams, version);
    return glassCreateContext(&defaultParams);
}

// Destroy context. Unbind if bound.
void glassDestroyContext(GLASSCtx ctx);

// Bind context. Pass NULL to unbind.
void glassBindContext(GLASSCtx ctx);

// Get bound context. UB if no bound context.
GLASSCtx glassGetBoundContext(void);

// Whether there's a bound context.
bool glassHasBoundContext(void);

// Check if that's the bound context.
bool glassIsBoundContext(GLASSCtx ctx);

// Get context version.
GLASSVersion glassGetVersion(GLASSCtx ctx);

// Get target screen.
GLASSScreen glassGetTargetScreen(GLASSCtx ctx);

// Set target screen.
void glassSetTargetScreen(GLASSCtx ctx, GLASSScreen screen);

// Get target side.
GLASSSide glassGetTargetSide(GLASSCtx ctx);

// Set target side, flushes pending GPU commands if different.
void glassSetTargetSide(GLASSCtx ctx, GLASSSide side);

// Swap buffers of the bound context. UB if no bound context.
void glassSwapBuffers(void);

// Swap buffers of the contexts. Can bind any of the two.
void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom);

void* glassHeapAlloc(size_t size);
void glassHeapFree(void* p);
size_t glassHeapSize(const void* p);
bool glassIsHeap(const void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(const void* p);
bool glassIsLinear(const void* p);

void* glassVRAMAlloc(size_t size, KYGXVRAMBank bank);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _GLASS_H */