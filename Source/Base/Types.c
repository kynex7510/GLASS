#include "Base/Types.h"
#include "Base/Utility.h"

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

    u32* obj = (u32*)glassVirtualAlloc(objSize);
    if (obj) {
        *obj = type;
        return (GLuint)obj;
    }

    return GLASS_INVALID_OBJECT;
}

bool GLASS_checkObjectType(GLuint obj, u32 type) {
    if (obj != GLASS_INVALID_OBJECT) {
#ifndef NDEBUG
        MemInfo minfo;
        PageInfo pinfo;
        svcQueryProcessMemory(&minfo, &pinfo, CUR_PROCESS_HANDLE, obj);
        ASSERT(minfo.perm & MEMPERM_READ);
#endif
        return *(u32*)obj == type;
    }

    return false;
}