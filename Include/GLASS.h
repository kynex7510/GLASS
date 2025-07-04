#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Defs.h>

typedef struct GLASSCtxImpl* GLASSCtx;

/// @brief OpenGL ES version.
typedef enum {
    GLASS_VERSION_2_0, ///< OpenGL ES 2.0
} GLASSVersion;

/// @brief Context initialization parameters.
typedef struct {
    GLASSVersion version;   ///< Context version.
    bool flushAllLinearMem; ///< Whether to flush all linear memory (default: true).
} GLASSInitParams;

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

/// @brief Context settings.
typedef struct {
    GLASSScreen targetScreen;       ///< Draw target screen (default: GLASS_SCREEN_TOP).
    GLASSSide targetSide;           ///< Draw target side (default: GLASS_SCREEN_LEFT).
    GLASSGPUCommandList GPUCmdList; ///< GPU command list (default: all NULL).
    bool vsync;                     ///< Enable VSync (default: true).
    bool horizontalFlip;            ///< Flip display buffer horizontally (default: false).
    GLASSDownscale downscale;       ///< Set downscale for anti-aliasing (default: GLASS_DOWNSCALE_NONE).
} GLASSSettings;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Create context.
GLASSCtx glassCreateContext(const GLASSInitParams* initParams, const GLASSSettings* settings);

// Create context with default settings.
GLASSCtx glassCreateDefaultContext(GLASSVersion version);

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

// Read settings from context.
void glassReadSettings(GLASSCtx ctx, GLASSSettings* settings);

// Write settings to context.
void glassWriteSettings(GLASSCtx ctx, const GLASSSettings* settings);

// Swap buffers of the bound context. UB if no bound context.
void glassSwapBuffers(void);

// Swap buffers of the contexts. Can bind any of the two if no bound context.
void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom);

void* glassHeapAlloc(size_t size);
void glassHeapFree(void* p);
size_t glassHeapSize(const void* p);
bool glassIsHeap(const void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(const void* p);
bool glassIsLinear(const void* p);

void* glassVRAMAlloc(KYGXVRAMBank bank, size_t size);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _GLASS_H */