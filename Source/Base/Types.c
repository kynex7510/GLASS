/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Types.h"

GLuint GLASS_createObject(u32 type) {
    size_t objSize = 0;

    switch (type) {
        case GLASS_BUFFER_TYPE:
            objSize = sizeof(BufferInfo);
            break;
        case GLASS_RENDERBUFFER_TYPE:
            objSize = sizeof(RenderbufferInfo);
            break;
        case GLASS_FRAMEBUFFER_TYPE:
            objSize = sizeof(FramebufferInfo);
            break;
        case GLASS_PROGRAM_TYPE:
            objSize = sizeof(ProgramInfo);
            break;
        case GLASS_SHADER_TYPE:
            objSize = sizeof(ShaderInfo);
            break;
        case GLASS_TEXTURE_TYPE:
            objSize = sizeof(TextureInfo);
            break;
        default:
            return GLASS_INVALID_OBJECT;
    }

    u32* obj = (u32*)glassHeapAlloc(objSize);
    if (obj) {
        *obj = type;
        return (GLuint)obj;
    }

    return GLASS_INVALID_OBJECT;
}