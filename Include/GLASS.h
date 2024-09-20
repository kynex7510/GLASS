#ifndef _GLASS_H
#define _GLASS_H

#include <3ds.h>

#define GLASS_VERSION_1_1 0x11 // OpenGL ES 1.1
// #define GLASS_VERSION_2_0 0x20 // OpenGL ES 2.0

typedef void* glassCtx;
typedef u32 glassVersion;

// Context settings.
typedef struct {
    gfxScreen_t targetScreen;        // Draw target screen (default: GFX_TOP).
    gfx3dSide_t targetSide;          // Draw target side (default: GFX_LEFT).
    GX_TRANSFER_SCALE transferScale; // Anti-aliasing (default: GX_TRANSFER_SCALE_NO).
} glassCtxSettings;

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

glassCtx glassCreateContext(glassVersion version);
glassCtx glassCreateContextWithSettings(glassVersion version, const glassCtxSettings* settings);
void glassDestroyContext(glassCtx ctx);
void glassReadSettings(glassCtx ctx, glassCtxSettings* settings);
void glassWriteSettings(glassCtx ctx, const glassCtxSettings* settings);
void glassBindContext(glassCtx ctx);
void glassSwapBuffers(void);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_H */