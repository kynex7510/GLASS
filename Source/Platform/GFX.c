#include "Platform/GFX.h"

#ifdef KYGX_BAREMETAL
// TODO
#else

u8* GLASS_gfx_getFramebuffer(GLASSScreen screen, GLASSSide side, u16* width, u16* height) {
    const gfxScreen_t nativeScreen = screen == GLASS_SCREEN_TOP ? GFX_TOP : GFX_BOTTOM;
    const gfx3dSide_t nativeSide = side == GLASS_SIDE_LEFT ? GFX_LEFT : GFX_RIGHT;
    return gfxGetFramebuffer(nativeScreen, nativeSide, width, height);
}

GLenum GLASS_gfx_getFramebufferFormat(GLASSScreen screen) {
    const GSPGPU_FramebufferFormat format = gfxGetScreenFormat(screen == GLASS_SCREEN_TOP ? GFX_TOP : GFX_BOTTOM);

    switch (format) {
        case GSP_RGBA8_OES:
            return GL_RGBA8_OES;
        case GSP_BGR8_OES:
            return GL_RGB8_OES;
        case GSP_RGB565_OES:
            return GL_RGB565;
        case GSP_RGB5_A1_OES:
            return GL_RGB5_A1;
        case GSP_RGBA4_OES:
            return GL_RGBA4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gfx_swapScreenBuffers(GLASSScreen screen, bool stereo) {
    const gfxScreen_t nativeScreen = screen == GLASS_SCREEN_TOP ? GFX_TOP : GFX_BOTTOM;
    gfxScreenSwapBuffers(nativeScreen, stereo);
}

#endif // KYGX_BAREMETAL