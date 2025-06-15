#ifndef _GLASS_BASE_CONTEXT_H
#define _GLASS_BASE_CONTEXT_H

#include "Base/Types.h"
#include "Platform/Sync.h"
#include "Platform/GX.h"

typedef struct {
    glassInitParams initParams; // Context parameters.
    glassSettings settings;     // Context settings.

    /* Platform */
    uint32_t flags;            // State flags.
    GLenum lastError;          // Actually first error.
    Barrier memoryFillBarrier; // Barrier used to wait on memory fills.
    GXCmdQueue gxQueue;        // GX command queue.

    /* Buffers */
    GLuint arrayBuffer;        // GL_ARRAY_BUFFER
    GLuint elementArrayBuffer; // GL_ELEMENT_ARRAY_BUFFER  

    /* Framebuffer */
    GLuint framebuffer;   // Bound framebuffer object.
    GLuint renderbuffer;  // Bound renderbuffer object.
    uint32_t clearColor;  // Color buffer clear value.
    GLclampf clearDepth;  // Depth buffer clear value.
    uint8_t clearStencil; // Stencil buffer clear value.
    bool block32;         // Block mode 32.

    /* Viewport */
    GLint viewportX;   // Viewport X.
    GLint viewportY;   // Viewport Y.
    GLsizei viewportW; // Viewport width.
    GLsizei viewportH; // Viewport height.

    /* Scissor */
    bool scissorEnabled;     // Scissor test.
    bool scissorInverted;    // Inverted scissor test.
    GLint scissorX;          // Scissor box X.
    GLint scissorY;          // Scissor box Y.
    GLsizei scissorW;        // Scissor box W.
    GLsizei scissorH;        // Scissor box H.

    /* Program */
    GLuint currentProgram; // Shader program in use.
    
    /* Attributes */
    AttributeInfo attribs[GLASS_NUM_ATTRIB_REGS]; // Attributes data.
    GLsizei numEnabledAttribs;                    // Number of enabled attributes.

    /* Fragment */
    GLenum fragMode; // Fragment mode.

    /* Color, Depth */
    bool writeRed;    // Write red.
    bool writeGreen;  // Write green.
    bool writeBlue;   // Write blue.
    bool writeAlpha;  // Write alpha.
    bool writeDepth;  // Write depth.
    bool depthTest;   // Depth test enabled.
    GLenum depthFunc; // Depth test function.

    /* Depth Map */
    GLclampf depthNear;    // Depth map near val.
    GLclampf depthFar;     // Depth map far val.
    bool polygonOffset;    // Depth map offset enabled.
    GLfloat polygonFactor; // Depth map factor (unused).
    GLfloat polygonUnits;  // Depth map offset units.

    /* Early Depth */
    bool earlyDepthTest;      // Early depth test enabled.
    GLclampf clearEarlyDepth; // Early depth clear value.
    GLenum earlyDepthFunc;    // Early depth function.

    /* Stencil */
    bool stencilTest;        // Stencil test enabled.
    GLenum stencilFunc;      // Stencil function.
    GLint stencilRef;        // Stencil reference value.
    GLuint stencilMask;      // Stencil mask.
    GLuint stencilWriteMask; // Stencil write mask.
    GLenum stencilFail;      // Stencil fail.
    GLenum stencilDepthFail; // Stencil pass, depth fail.
    GLenum stencilPass;      // Stencil pass + depth pass or no depth.

    /* Cull Face */
    bool cullFace;        // Cull face enabled.
    GLenum cullFaceMode;  // Cull face mode.
    GLenum frontFaceMode; // Front face mode.

    /* Alpha */
    bool alphaTest;    // Alpha test enabled.
    GLenum alphaFunc;  // Alpha test function.
    GLclampf alphaRef; // Alpha test reference value.

    /* Blend, Logic Op */
    bool blendMode;       // Blend mode.
    uint32_t blendColor;  // Blend color.
    GLenum blendEqRGB;    // Blend equation RGB.
    GLenum blendEqAlpha;  // Blend equation alpha.
    GLenum blendSrcRGB;   // Blend source RGB.
    GLenum blendDstRGB;   // Blend destination RGB.
    GLenum blendSrcAlpha; // Blend source alpha.
    GLenum blendDstAlpha; // Blend destination alpha.
    GLenum logicOp;       // Logic op.

    /* Texture */
    GLuint textureUnits[GLASS_NUM_TEX_UNITS]; // Texture units.
    size_t activeTextureUnit;                 // Currently active texture unit.  

    /* Combiners */
    GLint combinerStage;                               // Current combiner stage.
    CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES]; // Combiners.
} CtxCommon;

void GLASS_context_initCommon(CtxCommon* ctx, const glassInitParams* initParams, const glassSettings* settings);
void GLASS_context_cleanupCommon(CtxCommon* ctx);
CtxCommon* GLASS_context_getCommon(void);

void GLASS_context_bind(CtxCommon* ctx);
void GLASS_context_flush(CtxCommon* ctx, bool send);

#if defined(GLASS_NO_MERCY)
#define GLASS_context_setError(err) UNREACHABLE(#err)
#else
void GLASS_context_setError(GLenum error);
#endif // GLASS_NO_MERCY

#endif /* _GLASS_BASE_CONTEXT_H */