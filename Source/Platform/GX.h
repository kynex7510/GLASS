#ifndef _GLASS_PLATFORM_GX_H
#define _GLASS_PLATFORM_GX_H

#include "Base/Context.h"

void GLASS_gx_init(CtxCommon* ctx);
void GLASS_gx_cleanup(CtxCommon* ctx);
void GLASS_gx_bind(CtxCommon* ctx);
void GLASS_gx_unbind(CtxCommon* ctx);

void GLASS_gx_clearBuffers(RenderbufferInfo* colorBuffer, u32 clearColor, RenderbufferInfo* depthBuffer, u32 clearDepth);
void GLASS_gx_transferAndSwap(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer, void (*callback)(gxCmdQueue_s*));
void GLASS_gx_copyTexture(u32 srcAddr, u32 dstAddr, size_t size, size_t stride, size_t count);
void GLASS_gx_tiling(u32 srcAddr, u32 dstAddr, size_t width, size_t height, GLenum format, bool makeTiled);
void GLASS_gx_sendGPUCommands(void);

#endif /* _GLASS_PLATFORM_GX_H */