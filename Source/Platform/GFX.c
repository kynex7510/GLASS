/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Platform/GFX.h"

#ifdef KYGX_BAREMETAL

#include <drivers/gfx.h>

u8* GLASS_gfx_getFramebuffer(GLASSScreen screen, GLASSSide side, u16* width, u16* height) {
    const GfxLcd nativeScreen = screen == GLASS_SCREEN_TOP ? GFX_LCD_TOP : GFX_LCD_BOT;
    const GfxSide nativeSide = side == GLASS_SIDE_LEFT ? GFX_SIDE_LEFT : GFX_SIDE_RIGHT;

    u16 w;
    u16 h;
    if (nativeScreen == GFX_LCD_TOP) {
        w = LCD_WIDTH_TOP;
        h = GFX_getTopMode() == GFX_TOP_WIDE ? LCD_WIDE_HEIGHT_TOP : LCD_HEIGHT_TOP;
    } else if (nativeScreen == GFX_LCD_BOT) {
        w = LCD_WIDTH_BOT;
        h = LCD_HEIGHT_BOT;
    }

    if (width)
        *width = w;

    if (height)
        *height = h;

    return GFX_getBuffer(nativeScreen, nativeSide);
}

GLenum GLASS_gfx_getFramebufferFormat(GLASSScreen screen) {
    const GfxFmt format = GFX_getFormat(screen == GLASS_SCREEN_TOP ? GFX_LCD_TOP : GFX_LCD_BOT);
    
    switch (format) {
        case GFX_ABGR8:
            return GL_RGBA8_OES;
        case GFX_BGR8:
            return GL_RGB8_OES;
        case GFX_BGR565:
            return GL_RGB565;
        case GFX_A1BGR5:
            return GL_RGB5_A1;
        case GFX_ABGR4:
            return GL_RGBA4;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gfx_swapScreenBuffers(GLASSScreen screen) { GFX_swapBuffers(); }

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

void GLASS_gfx_swapScreenBuffers(GLASSScreen screen) {
    const gfxScreen_t nativeScreen = screen == GLASS_SCREEN_TOP ? GFX_TOP : GFX_BOTTOM;
    const bool stereo = screen == GLASS_SCREEN_TOP && osGet3DSliderState() > 0.0f;
    gfxScreenSwapBuffers(nativeScreen, stereo);
}

#endif // KYGX_BAREMETAL