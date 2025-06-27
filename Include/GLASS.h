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

GLASSCtx glassCreateContext(const GLASSInitParams* initParams, const GLASSSettings* settings);
GLASSCtx glassCreateDefaultContext(GLASSVersion version);
void glassDestroyContext(GLASSCtx ctx);
void glassBindContext(GLASSCtx ctx);

void glassReadSettings(GLASSCtx ctx, GLASSSettings* settings);
void glassWriteSettings(GLASSCtx ctx, const GLASSSettings* settings);

void glassSwapBuffers(void);
void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom);

void* glassHeapAlloc(size_t size);
void glassHeapFree(void* p);
size_t glassHeapSize(const void* p);
bool glassIsHeap(const void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(const void* p);
bool glassIsLinear(const void* p);

void* glassVRAMAlloc(GXVRAMBank bank, size_t size);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _GLASS_H */