#ifndef _GLASS_GX_H
#define _GLASS_GX_H

#include "Base/Context.h"

typedef struct {
    u32 srcAddr;
    u32 dstAddr;
    size_t size;
    size_t stride;
    size_t count;
} TexCopyParams;

typedef struct {
    GLsizei inputWidth;
    GLsizei inputHeight;
    GLenum inputFormat;
    GLsizei outputWidth;
    GLsizei outputHeight;
    GLenum outputFormat;
    bool verticalFlip;
    bool makeTiled;
} TexTransformationParams;

void GLASS_gx_init(CtxCommon* ctx);
void GLASS_gx_cleanup(CtxCommon* ctx);
void GLASS_gx_bind(CtxCommon* ctx);
void GLASS_gx_unbind(CtxCommon* ctx);

void GLASS_gx_clearBuffers(RenderbufferInfo* colorBuffer, u32 clearColor, RenderbufferInfo* depthBuffer, u32 clearDepth);
void GLASS_gx_transferAndSwap(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer);
void GLASS_gx_copyTexture(const TexCopyParams* params);
void GLASS_gx_transformTexture(u32 srcAddr, u32 dstAddr, const TexTransformationParams* params);
void GLASS_gx_sendGPUCommands(void);

#endif /* _GLASS_GX_H */