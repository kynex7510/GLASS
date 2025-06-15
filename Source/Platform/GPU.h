#ifndef _GLASS_PLATFORM_GPU_H
#define _GLASS_PLATFORM_GPU_H

#include "Base/Context.h"

void GLASS_gpu_allocList(glassGPUCommandList* list);
void GLASS_gpu_freeList(glassGPUCommandList* list);
bool GLASS_gpu_swapListBuffers(glassGPUCommandList* list, void** outBuffer, size_t* outSize);

void GLASS_gpu_bindFramebuffer(glassGPUCommandList* list, const FramebufferInfo* info, bool block32);
void GLASS_gpu_flushFramebuffer(glassGPUCommandList* list);
void GLASS_gpu_invalidateFramebuffer(glassGPUCommandList* list);

void GLASS_gpu_setViewport(glassGPUCommandList* list, GLint x, GLint y, GLsizei width, GLsizei height);
void GLASS_gpu_setScissorTest(glassGPUCommandList* list, GPU_SCISSORMODE mode, GLint x, GLint y, GLsizei width, GLsizei height);

void GLASS_gpu_bindShaders(glassGPUCommandList* list, const ShaderInfo* vertexShader, const ShaderInfo* geometryShader);
void GLASS_gpu_uploadConstUniforms(glassGPUCommandList* list, const ShaderInfo* shader);
void GLASS_gpu_uploadUniforms(glassGPUCommandList* list, ShaderInfo* shader);

void GLASS_gpu_uploadAttributes(glassGPUCommandList* list, const AttributeInfo* attribs);

void GLASS_gpu_setCombiners(glassGPUCommandList* list, const CombinerInfo* combiners);

void GLASS_gpu_setFragOp(glassGPUCommandList* list, GLenum mode, bool blendMode);
void GLASS_gpu_setColorDepthMask(glassGPUCommandList* list, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc);
void GLASS_gpu_setDepthMap(glassGPUCommandList* list, bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum depthFormat);
void GLASS_gpu_setEarlyDepthTest(glassGPUCommandList* list, bool enabled);
void GLASS_gpu_setEarlyDepthFunc(glassGPUCommandList* list, GPU_EARLYDEPTHFUNC func);
void GLASS_gpu_setEarlyDepthClear(glassGPUCommandList* list, GLclampf value);
void GLASS_gpu_clearEarlyDepthBuffer(glassGPUCommandList* list);
void GLASS_gpu_setStencilTest(glassGPUCommandList* list, bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask);
void GLASS_gpu_setStencilOp(glassGPUCommandList* list, GLenum sfail, GLenum dpfail, GLenum dppass);
void GLASS_gpu_setCullFace(glassGPUCommandList* list, bool enabled, GLenum cullFace, GLenum frontFace);
void GLASS_gpu_setAlphaTest(glassGPUCommandList* list, bool enabled, GLenum func, GLclampf ref);
void GLASS_gpu_setBlendFunc(glassGPUCommandList* list, GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha);
void GLASS_gpu_setBlendColor(glassGPUCommandList* list, u32 color);
void GLASS_gpu_setLogicOp(glassGPUCommandList* list, GLenum logicOp);

void GLASS_gpu_drawArrays(glassGPUCommandList* list, GLenum mode, GLint first, GLsizei count);
void GLASS_gpu_drawElements(glassGPUCommandList* list, GLenum mode, GLsizei count, GLenum type, u32 physIndices);

void GLASS_gpu_setTextureUnits(glassGPUCommandList* list, const GLuint* units);

#endif /* _GLASS_PLATFORM_GPU_H */