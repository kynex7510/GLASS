#ifndef _GLASS_PLATFORM_GPU_H
#define _GLASS_PLATFORM_GPU_H

#include "Base/Types.h"

void GLASS_gpu_allocList(GLASSGPUCommandList* list);
void GLASS_gpu_freeList(GLASSGPUCommandList* list);
bool GLASS_gpu_swapListBuffers(GLASSGPUCommandList* list, void** outBuffer, size_t* outSize);

void GLASS_gpu_bindFramebuffer(GLASSGPUCommandList* list, const FramebufferInfo* info, bool block32);
void GLASS_gpu_flushFramebuffer(GLASSGPUCommandList* list);
void GLASS_gpu_invalidateFramebuffer(GLASSGPUCommandList* list);

void GLASS_gpu_setViewport(GLASSGPUCommandList* list, GLint x, GLint y, GLsizei width, GLsizei height);
void GLASS_gpu_setScissorTest(GLASSGPUCommandList* list, GPUScissorMode mode, GLint x, GLint y, GLsizei width, GLsizei height);

void GLASS_gpu_bindShaders(GLASSGPUCommandList* list, const ShaderInfo* vertexShader, const ShaderInfo* geometryShader);
void GLASS_gpu_uploadConstUniforms(GLASSGPUCommandList* list, const ShaderInfo* shader);
void GLASS_gpu_uploadUniforms(GLASSGPUCommandList* list, ShaderInfo* shader);

void GLASS_gpu_uploadAttributes(GLASSGPUCommandList* list, const AttributeInfo* attribs);

void GLASS_gpu_setCombiners(GLASSGPUCommandList* list, const CombinerInfo* combiners);

void GLASS_gpu_setFragOp(GLASSGPUCommandList* list, GLenum mode, bool blendMode);
void GLASS_gpu_setColorDepthMask(GLASSGPUCommandList* list, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc);
void GLASS_gpu_setDepthMap(GLASSGPUCommandList* list, bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum depthFormat);
void GLASS_gpu_setEarlyDepthTest(GLASSGPUCommandList* list, bool enabled);
void GLASS_gpu_setEarlyDepthFunc(GLASSGPUCommandList* list, GPUEarlyDepthFunc func);
void GLASS_gpu_setEarlyDepthClear(GLASSGPUCommandList* list, GLclampf value);
void GLASS_gpu_clearEarlyDepthBuffer(GLASSGPUCommandList* list);
void GLASS_gpu_setStencilTest(GLASSGPUCommandList* list, bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask);
void GLASS_gpu_setStencilOp(GLASSGPUCommandList* list, GLenum sfail, GLenum dpfail, GLenum dppass);
void GLASS_gpu_setCullFace(GLASSGPUCommandList* list, bool enabled, GLenum cullFace, GLenum frontFace);
void GLASS_gpu_setAlphaTest(GLASSGPUCommandList* list, bool enabled, GLenum func, GLclampf ref);
void GLASS_gpu_setBlendFunc(GLASSGPUCommandList* list, GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha);
void GLASS_gpu_setBlendColor(GLASSGPUCommandList* list, u32 color);
void GLASS_gpu_setLogicOp(GLASSGPUCommandList* list, GLenum logicOp);

void GLASS_gpu_drawArrays(GLASSGPUCommandList* list, GLenum mode, GLint first, GLsizei count);
void GLASS_gpu_drawElements(GLASSGPUCommandList* list, GLenum mode, GLsizei count, GLenum type, u32 physIndices);

void GLASS_gpu_setTextureUnits(GLASSGPUCommandList* list, const GLuint* units);

#endif /* _GLASS_PLATFORM_GPU_H */