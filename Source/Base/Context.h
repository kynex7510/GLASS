/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _GLASS_BASE_CONTEXT_H
#define _GLASS_BASE_CONTEXT_H

#include "Base/Types.h"

/*
typedef struct {
    GLASSCtxParams params; // Context parameters.
    u32 flags;             // State flags.
    GLenum lastError;      // Actually first error.

    // Platform
    KYGXCmdBuffer GXCmdBuf;    // GX command buffer.
    VSyncBarrier vsyncBarrier; // VSync barrier.

    // Buffers
    GLuint arrayBuffer;        // GL_ARRAY_BUFFER
    GLuint elementArrayBuffer; // GL_ELEMENT_ARRAY_BUFFER  

    // Framebuffer
    GLuint framebuffer[2]; // Bound framebuffer object.
    GLuint renderbuffer;   // Bound renderbuffer object.
    u32 clearColor;        // Color buffer clear value.
    GLclampf clearDepth;   // Depth buffer clear value.
    u8 clearStencil;       // Stencil buffer clear value.
    bool block32;          // Block mode 32.

    // Viewport
    GLint viewportX;   // Viewport X.
    GLint viewportY;   // Viewport Y.
    GLsizei viewportW; // Viewport width.
    GLsizei viewportH; // Viewport height.

    // Scissor
    GPUScissorMode scissorMode; // Scissor test mode.
    GLint scissorX;             // Scissor box X.
    GLint scissorY;             // Scissor box Y.
    GLsizei scissorW;           // Scissor box W.
    GLsizei scissorH;           // Scissor box H.

    // Program
    GLuint currentProgram; // Shader program in use.
    
    // Attributes
    AttributeInfo attribs[GLASS_NUM_ATTRIB_REGS]; // Attributes data.
    GLsizei numEnabledAttribs;                    // Number of enabled attributes.

    // Fragment
    GLenum fragMode; // Fragment mode.

    // Color, Depth
    bool writeRed;    // Write red.
    bool writeGreen;  // Write green.
    bool writeBlue;   // Write blue.
    bool writeAlpha;  // Write alpha.
    bool writeDepth;  // Write depth.
    bool depthTest;   // Depth test enabled.
    GLenum depthFunc; // Depth test function.

    // Depth Map
    GLclampf depthNear;    // Depth map near val.
    GLclampf depthFar;     // Depth map far val.
    bool polygonOffset;    // Depth map offset enabled.
    GLfloat polygonFactor; // Depth map factor (unused).
    GLfloat polygonUnits;  // Depth map offset units.

    // Early Depth
    bool earlyDepthTest;      // Early depth test enabled.
    GLclampf clearEarlyDepth; // Early depth clear value.
    GLenum earlyDepthFunc;    // Early depth function.

    // Stencil
    bool stencilTest;        // Stencil test enabled.
    GLenum stencilFunc;      // Stencil function.
    GLint stencilRef;        // Stencil reference value.
    GLuint stencilMask;      // Stencil mask.
    GLuint stencilWriteMask; // Stencil write mask.
    GLenum stencilFail;      // Stencil fail.
    GLenum stencilDepthFail; // Stencil pass, depth fail.
    GLenum stencilPass;      // Stencil pass + depth pass or no depth.

    // Cull Face
    bool cullFace;        // Cull face enabled.
    GLenum cullFaceMode;  // Cull face mode.
    GLenum frontFaceMode; // Front face mode.

    // Alpha
    bool alphaTest;    // Alpha test enabled.
    GLenum alphaFunc;  // Alpha test function.
    GLclampf alphaRef; // Alpha test reference value.

    // Blend, Logic Op
    bool blendMode;       // Blend mode.
    u32 blendColor;       // Blend color.
    GLenum blendEqRGB;    // Blend equation RGB.
    GLenum blendEqAlpha;  // Blend equation alpha.
    GLenum blendSrcRGB;   // Blend source RGB.
    GLenum blendDstRGB;   // Blend destination RGB.
    GLenum blendSrcAlpha; // Blend source alpha.
    GLenum blendDstAlpha; // Blend destination alpha.
    GLenum logicOp;       // Logic op.

    // Texture
    GLuint textureUnits[GLASS_NUM_TEX_UNITS]; // Texture units.
    size_t activeTextureUnit;                 // Currently active texture unit.  

    // Combiners
    GLint combinerStage;                               // Current combiner stage.
    CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES]; // Combiners.
} CtxCommon;
*/

typedef struct {
    u32 flags;
    GLenum lastError;
    GLuint arrayBuffer;
    GLuint elementArrayBuffer;
    GLuint renderbuffer;
    u32 clearColor;
    GLint viewportX;
    GLint viewportY;
    GLsizei viewportW;
    GLsizei viewportH;
    GLint scissorX;
    GLint scissorY;
    GLsizei scissorW;
    GLsizei scissorH;
    GLuint currentProgram;
    GLsizei numEnabledAttribs;
    GLenum fragMode;
    GLenum depthFunc;
    GLenum earlyDepthFunc;
    GLenum stencilFunc;
    GLint stencilRef;
    GLuint stencilMask;
    GLuint stencilWriteMask;
    GLenum stencilFail;
    GLenum stencilDepthFail; 
    GLenum stencilPass;
    GLenum cullFaceMode;
    GLenum frontFaceMode;
    GLenum alphaFunc;
    u32 blendColor;
    GLenum blendEqRGB;
    GLenum blendEqAlpha;
    GLenum blendSrcRGB;
    GLenum blendDstRGB;
    GLenum blendSrcAlpha;
    GLenum blendDstAlpha;
    GLenum logicOp;
    GLint combinerStage;
    size_t activeTextureUnit;
    GLuint framebuffer[2];
    GLuint textureUnits[GLASS_NUM_TEX_UNITS];
    KYGXCmdBuffer GXCmdBuf;
    VSyncBarrier vsyncBarrier;
    GLASSCtxParams params;
    CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES];
    AttributeInfo attribs[GLASS_NUM_ATTRIB_REGS];
    GLclampf clearDepth;
    GPUScissorMode scissorMode;
    GLclampf depthNear;
    GLclampf depthFar;
    GLfloat polygonFactor;
    GLfloat polygonUnits;
    GLclampf clearEarlyDepth;
    GLclampf alphaRef;
    u8 clearStencil;
    bool block32;
    bool writeRed;
    bool writeGreen;
    bool writeBlue;
    bool writeAlpha;
    bool writeDepth;
    bool depthTest;
    bool polygonOffset;
    bool earlyDepthTest;
    bool stencilTest;
    bool cullFace;
    bool alphaTest;
    bool blendMode;
} CtxCommon;

void GLASS_context_initCommon(CtxCommon* ctx, const GLASSCtxParams* ctxParams);
void GLASS_context_cleanupCommon(CtxCommon* ctx);

CtxCommon* GLASS_context_getBound(void);
bool GLASS_context_hasBound(void);
bool GLASS_context_isBound(CtxCommon* ctx);

void GLASS_context_bind(CtxCommon* ctx);
void GLASS_context_flush(CtxCommon* ctx, bool send);

#if defined(GLASS_NO_MERCY)
#define GLASS_context_setError(err) KYGX_UNREACHABLE(#err)
#else
void GLASS_context_setError(GLenum error);
#endif // GLASS_NO_MERCY

static inline size_t GLASS_context_getFBIndex(CtxCommon* ctx) {
    KYGX_ASSERT(ctx);
    return (ctx->params.targetSide == GLASS_SIDE_RIGHT) ? 1 : 0;
}

#endif /* _GLASS_BASE_CONTEXT_H */