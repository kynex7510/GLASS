#ifndef _GLASS_PLATFORM_GPU_H
#define _GLASS_PLATFORM_GPU_H

#include "Base/Types.h"

void GLASS_gpu_allocList(glassGpuCommandList* list);
void GLASS_gpu_freeList(glassGpuCommandList* list);
bool GLASS_gpu_swapCommandBuffers(glassGpuCommandList* list, void** outBuffer, size_t* outSize);

void GLASS_gpu_bindFramebuffer(glassGpuCommandList* list, const FramebufferInfo* info, bool block32);
void GLASS_gpu_flushFramebuffer(glassGpuCommandList* list);
void GLASS_gpu_invalidateFramebuffer(glassGpuCommandList* list);

void GLASS_gpu_setViewport(glassGpuCommandList* list, GLint x, GLint y, GLsizei width, GLsizei height);
void GLASS_gpu_setScissorTest(glassGpuCommandList* list, bool enabled, bool inverted, GLint x, GLint y, GLsizei width, GLsizei height);

void GLASS_gpu_bindShaders(glassGpuCommandList* list, const ShaderInfo* vertexShader, const ShaderInfo* geometryShader);
void GLASS_gpu_uploadConstUniforms(glassGpuCommandList* list, const ShaderInfo* shader);
void GLASS_gpu_uploadUniforms(glassGpuCommandList* list, ShaderInfo* shader);

void GLASS_gpu_uploadAttributes(glassGpuCommandList* list, const AttributeInfo* attribs);

void GLASS_gpu_setCombiners(glassGpuCommandList* list, const CombinerInfo* combiners);

void GLASS_gpu_setFragOp(glassGpuCommandList* list, GLenum mode, bool blendMode);
void GLASS_gpu_setColorDepthMask(glassGpuCommandList* list, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc);
void GLASS_gpu_setDepthMap(glassGpuCommandList* list, bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum depthFormat);
void GLASS_gpu_setEarlyDepthTest(glassGpuCommandList* list, bool enabled);
void GLASS_gpu_setEarlyDepthFunc(glassGpuCommandList* list, GLenum func);
void GLASS_gpu_setEarlyDepthClear(glassGpuCommandList* list, GLclampf value);
void GLASS_gpu_clearEarlyDepthBuffer(glassGpuCommandList* list);
void GLASS_gpu_setStencilTest(glassGpuCommandList* list, bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask);
void GLASS_gpu_setStencilOp(glassGpuCommandList* list, GLenum sfail, GLenum dpfail, GLenum dppass);
void GLASS_gpu_setCullFace(glassGpuCommandList* list, bool enabled, GLenum cullFace, GLenum frontFace);
void GLASS_gpu_setAlphaTest(glassGpuCommandList* list, bool enabled, GLenum func, GLclampf ref);
void GLASS_gpu_setBlendFunc(glassGpuCommandList* list, GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha);
void GLASS_gpu_setBlendColor(glassGpuCommandList* list, uint32_t color);
void GLASS_gpu_setLogicOp(glassGpuCommandList* list, GLenum logicOp);

void GLASS_gpu_drawArrays(glassGpuCommandList* list, GLenum mode, GLint first, GLsizei count);
void GLASS_gpu_drawElements(glassGpuCommandList* list, GLenum mode, GLsizei count, GLenum type, uint32_t physIndices);

void GLASS_gpu_setTextureUnits(glassGpuCommandList* list, const GLuint* units);

#endif /* _GLASS_PLATFORM_GPU_H */