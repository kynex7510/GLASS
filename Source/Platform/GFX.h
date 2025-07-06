#ifndef _GLASS_PLATFORM_GFX_H
#define _GLASS_PLATFORM_GFX_H

#include "Base/Types.h"

u8* GLASS_gfx_getFramebuffer(GLASSScreen screen, GLASSSide side, u16* width, u16* height);
GLenum GLASS_gfx_getFramebufferFormat(GLASSScreen screen);
void GLASS_gfx_swapScreenBuffers(GLASSScreen screen);

#endif /* _GLASS_PLATFORM_GFX_H */