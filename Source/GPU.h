#ifndef _GLASS_GPU_H
#define _GLASS_GPU_H

#include "Context.h"

void GLASS_gpu_init(CtxCommon* ctx);
void GLASS_gpu_finalize(CtxCommon* ctx);

void GLASS_gpu_enableCommands(CtxCommon* ctx);
void GLASS_gpu_disableCommands(CtxCommon* ctx);

void GLASS_gpu_flushQueue(CtxCommon* ctx, bool unbind);
void GLASS_gpu_runQueue(CtxCommon* ctx, bool bind);
void GLASS_gpu_flushCommands(CtxCommon* ctx);
void GLASS_gpu_flushAndRunCommands(CtxCommon* ctx);

void GLASS_gpu_clearBuffers(RenderbufferInfo* colorBuffer, u32 clearColor, RenderbufferInfo* depthBuffer, u32 clearDepth);
void GLASS_gpu_transferBuffer(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer, u32 flags);
void GLASS_gpu_bindFramebuffer(const FramebufferInfo* info, bool block32);
void GLASS_gpu_flushFramebuffer(void);
void GLASS_gpu_invalidateFramebuffer(void);

void GLASS_gpu_setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void GLASS_gpu_setScissorTest(GPU_SCISSORMODE mode, GLint x, GLint y, GLsizei width, GLsizei height);

void GLASS_gpu_bindShaders(const ShaderInfo* vertexShader, const ShaderInfo* geometryShader);
void GLASS_gpu_uploadConstUniforms(const ShaderInfo* shader);
void GLASS_gpu_uploadUniforms(ShaderInfo* shader);

void GLASS_gpu_uploadAttributes(const AttributeInfo* attribs);

void GLASS_gpu_setCombiners(const CombinerInfo* combiners);

void GLASS_gpu_setFragOp(GLenum fragMode, bool blendMode);
void GLASS_gpu_setColorDepthMask(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc);
void GLASS_gpu_setDepthMap(bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum depthFormat);
void GLASS_gpu_setEarlyDepthTest(bool enabled);
void GLASS_gpu_setEarlyDepthFunc(GPU_EARLYDEPTHFUNC func);
void GLASS_gpu_setEarlyDepthClear(GLclampf value);
void GLASS_gpu_clearEarlyDepthBuffer(void);
void GLASS_gpu_setStencilTest(bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask);
void GLASS_gpu_setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void GLASS_gpu_setCullFace(bool enabled, GLenum cullFace, GLenum frontFace);
void GLASS_gpu_setAlphaTest(bool enabled, GLenum func, GLclampf ref);
void GLASS_gpu_setBlendFunc(GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha);
void GLASS_gpu_setBlendColor(u32 color);
void GLASS_gpu_setLogicOp(GLenum logicOp);

void GLASS_gpu_drawArrays(GLenum mode, GLint first, GLsizei count);
void GLASS_gpu_drawElements(GLenum mode, GLsizei count, GLenum type, u32 physIndices);

void GLASS_gpu_setTextureUnits(const TextureUnit* units);

#endif /* _GLASS_GPU_H */