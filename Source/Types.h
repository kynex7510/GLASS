#ifndef _GLASS_TYPES_H
#define _GLASS_TYPES_H

#include "GLASS.h"
#include "GLASS/Defs.h"

#include <stddef.h>

#define DECL_FLAG(id) (u32)(1 << (id))

#define GLASS_INVALID_OBJECT 0
#define GLASS_NUM_ATTRIB_SLOTS 12
#define GLASS_NUM_ATTRIB_REGS 16
#define GLASS_NUM_BOOL_UNIFORMS 16
#define GLASS_NUM_INT_UNIFORMS 4
#define GLASS_NUM_FLOAT_UNIFORMS 96
#define GLASS_NUM_COMBINER_STAGES 6
#define GLASS_UNI_BOOL 0x00
#define GLASS_UNI_INT 0x01
#define GLASS_UNI_FLOAT 0x02

#define GLASS_UNKNOWN_TYPE 0x00
#define GLASS_BUFFER_TYPE 0x01
#define GLASS_TEXTURE_TYPE 0x02
#define GLASS_PROGRAM_TYPE 0x03
#define GLASS_SHADER_TYPE 0x04
#define GLASS_FRAMEBUFFER_TYPE 0x05
#define GLASS_RENDERBUFFER_TYPE 0x06

#define BUFFER_FLAG_BOUND DECL_FLAG(0)

#define RENDERBUFFER_FLAG_BOUND DECL_FLAG(0)

#define FRAMEBUFFER_FLAG_BOUND DECL_FLAG(0)

#define SHADER_FLAG_DELETE DECL_FLAG(0)
#define SHADER_FLAG_GEOMETRY DECL_FLAG(1)
#define SHADER_FLAG_MERGE_OUTMAPS DECL_FLAG(2)
#define SHADER_FLAG_USE_TEXCOORDS DECL_FLAG(3)

#define PROGRAM_FLAG_DELETE DECL_FLAG(0)
#define PROGRAM_FLAG_LINK_FAILED DECL_FLAG(1)
#define PROGRAM_FLAG_UPDATE_VERTEX DECL_FLAG(2)
#define PROGRAM_FLAG_UPDATE_GEOMETRY DECL_FLAG(3)

#define CONTEXT_FLAG_FRAMEBUFFER DECL_FLAG(0)
#define CONTEXT_FLAG_DRAW DECL_FLAG(1)
#define CONTEXT_FLAG_VIEWPORT DECL_FLAG(2)
#define CONTEXT_FLAG_SCISSOR DECL_FLAG(3)
#define CONTEXT_FLAG_ATTRIBS DECL_FLAG(4)
#define CONTEXT_FLAG_PROGRAM DECL_FLAG(5)
#define CONTEXT_FLAG_COMBINERS DECL_FLAG(6)
#define CONTEXT_FLAG_FRAGMENT DECL_FLAG(7)
#define CONTEXT_FLAG_DEPTHMAP DECL_FLAG(8)
#define CONTEXT_FLAG_COLOR_DEPTH DECL_FLAG(9)
#define CONTEXT_FLAG_EARLY_DEPTH DECL_FLAG(10)
#define CONTEXT_FLAG_EARLY_DEPTH_CLEAR DECL_FLAG(11)
#define CONTEXT_FLAG_STENCIL DECL_FLAG(12)
#define CONTEXT_FLAG_CULL_FACE DECL_FLAG(13)
#define CONTEXT_FLAG_ALPHA DECL_FLAG(14)
#define CONTEXT_FLAG_BLEND DECL_FLAG(15)

#define OBJ_IS_BUFFER(x) GLASS_checkObjectType((x), GLASS_BUFFER_TYPE)
#define OBJ_IS_TEXTURE(x) GLASS_checkObjectType((x), GLASS_TEXTURE_TYPE)
#define OBJ_IS_PROGRAM(x) GLASS_checkObjectType((x), GLASS_PROGRAM_TYPE)
#define OBJ_IS_SHADER(x) GLASS_checkObjectType((x), GLASS_SHADER_TYPE)
#define OBJ_IS_FRAMEBUFFER(x) GLASS_checkObjectType((x), GLASS_FRAMEBUFFER_TYPE)
#define OBJ_IS_RENDERBUFFER(x) GLASS_checkObjectType((x), GLASS_RENDERBUFFER_TYPE)

typedef struct {
    u32 type;     // GL type (GLASS_BUFFER_TYPE).
    u8* address;  // Data address.
    GLenum usage; // Buffer usage type.
    u16 flags;    // Buffer flags.
} BufferInfo;

typedef struct {
    u32 type;       // GL type (GLASS_RENDERBUFFER_TYPE).
    u8* address;    // Data address.
    GLsizei width;  // Buffer width.
    GLsizei height; // Buffer height.
    GLenum format;  // Buffer format.
    u16 flags;      // Renderbuffer flags.
} RenderbufferInfo;

typedef struct {
    u32 type;                      // GL type (GLASS_FRAMEBUFFER_TYPE).
    RenderbufferInfo* colorBuffer; // Bound color buffer.
    RenderbufferInfo* depthBuffer; // Bound depth (+ stencil) buffer.
    u32 flags;                     // Framebuffer flags.
} FramebufferInfo;

typedef struct {
    GLenum type;           // Type of each component.
    GLint count;           // Num of components.
    GLsizei stride;        // Num of all bytes.
    GLuint boundBuffer;    // Bound array buffer.
    u32 physAddr;          // Physical address to buffer.
    GLfloat components[4]; // Fixed attrib X-Y-Z-W.
} AttributeInfo;

typedef struct {
    u32 refc;           // Reference count.
    u32* binaryCode;    // Binary code buffer.
    u32 numOfCodeWords; // Num of instructions.
    u32* opDescs;       // Operand descriptors.
    u32 numOfOpDescs;   // Num of operand descriptors.
} SharedShaderData;

typedef struct {
    u8 ID;           // Uniform ID.
    u8 type;         // Uniform type.
    size_t count;    // Number of elements.
    char* symbol;    // Pointer to symbol.
    union {
        u16 mask;    // Bool, bool array mask.
        u32 value;   // Int value.
        u32* values; // int array, float, float array data.
    } data;
    bool dirty; // Uniform dirty.
} UniformInfo;

// Represents a constant float uniform.
typedef struct {
    u8 ID;       // Constant ID.
    u32 data[3]; // Constant data.
} ConstFloatInfo;

typedef struct {
    u32 type;                           // GL type (GLASS_SHADER_TYPE).
    SharedShaderData* sharedData;       // Shared shader data.
    size_t codeEntrypoint;              // Code entrypoint.
    DVLE_geoShaderMode gsMode;          // Mode for geometry shader.
    u16 outMask;                        // Used output registers mask.
    u16 outTotal;                       // Total number of output registers.
    u32 outSems[7];                     // Output register semantics.
    u32 outClock;                       // Output register clock.
    char* symbolTable;                  // This shader symbol table.
    u32 sizeOfSymbolTable;              // Size of symbol table.
    u16 constBoolMask;                  // Constant bool uniform mask.
    u32 constIntData[4];                // Constant int uniform data.
    u16 constIntMask;                   // Constant int uniform mask.
    ConstFloatInfo* constFloatUniforms; // Constant uniforms.
    u32 numOfConstFloatUniforms;        // Num of const uniforms.
    UniformInfo* activeUniforms;        // Active uniforms.
    u32 numOfActiveUniforms;            // Num of active uniforms.
    u16 flags;                          // Shader flags.
    u16 refc;                           // Reference count.
} ShaderInfo;

typedef struct {
  u32 type; // GL type (GLASS_PROGRAM_TYPE).
  // Attached = result of glAttachShader.
  // Linked = result of glLinkProgram.
  GLuint attachedVertex;   // Attached vertex shader.
  GLuint linkedVertex;     // Linked vertex shader.
  GLuint attachedGeometry; // Attached geometry shader.
  GLuint linkedGeometry;   // Linked geometry shader.
  u32 flags;               // Program flags.
} ProgramInfo;

typedef struct {
  GLenum rgbSrc[3];   // RGB source 0-1-2.
  GLenum alphaSrc[3]; // Alpha source 0-1-2.
  GLenum rgbOp[3];    // RGB operand 0-1-2.
  GLenum alphaOp[3];  // Alpha operand 0-1-2.
  GLenum rgbFunc;     // RGB function.
  GLenum alphaFunc;   // Alpha function.
  GLfloat rgbScale;   // RGB scale.
  GLfloat alphaScale; // Alpha scale.
  u32 color;          // Constant color.
} CombinerInfo;

typedef struct {
    glassVersion version;      // Context version.
    glassSettings settings; // Context settings.

    /* Platform */
    u32 flags;            // State flags.
    GLenum lastError;     // Actually first error.
    u32* cmdBuffer;       // Buffer for GPU commands.
    u32 cmdBufferSize;    // Offset relative to cmd buffer offset.
    u32 cmdBufferOffset;  // Offset relative to cmd buffer base.
    gxCmdQueue_s gxQueue; // Queue for GX commands.
    u8 inSwap;            // Whether we're currently swapping.

    /* Buffers */
    GLuint arrayBuffer;        // GL_ARRAY_BUFFER
    GLuint elementArrayBuffer; // GL_ELEMENT_ARRAY_BUFFER

    /* Framebuffer */
    GLuint framebuffer;  // Bound framebuffer object.
    GLuint renderbuffer; // Bound renderbuffer object.
    u32 clearColor;      // Color buffer clear value.
    GLclampf clearDepth; // Depth buffer clear value.
    u8 clearStencil;     // Stencil buffer clear value.
    bool block32;        // Block mode 32.

    /* Viewport */
    GLint viewportX;   // Viewport X.
    GLint viewportY;   // Viewport Y.
    GLsizei viewportW; // Viewport width.
    GLsizei viewportH; // Viewport height.

    /* Scissor */
    GPU_SCISSORMODE scissorMode; // Scissor test mode.
    GLint scissorX;              // Scissor box X.
    GLint scissorY;              // Scissor box Y.
    GLsizei scissorW;            // Scissor box W.
    GLsizei scissorH;            // Scissor box H.

    /* Program */
    GLuint currentProgram; // Shader program in use.
    
    /* Attributes */
    AttributeInfo attribs[GLASS_NUM_ATTRIB_REGS]; // Attributes data.
    size_t attribSlots[GLASS_NUM_ATTRIB_SLOTS];   // Attributes slots.

    /* Combiners */
    GLint combinerStage;                               // Current combiner stage.
    CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES]; // Combiners.

    /* Fragment */
    GLenum fragMode; // Fragment mode.
    bool blendMode;  // Blend mode.

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

    /* Blend */
    u32 blendColor;       // Blend color.
    GLenum blendEqRGB;    // Blend equation RGB.
    GLenum blendEqAlpha;  // Blend equation alpha.
    GLenum blendSrcRGB;   // Blend source RGB.
    GLenum blendDstRGB;   // Blend destination RGB.
    GLenum blendSrcAlpha; // Blend source alpha.
    GLenum blendDstAlpha; // Blend destination alpha.

    /* Logic Op */
    GLenum logicOp; // Logic op.
} CtxCommon;

GLuint GLASS_createObject(u32 type);
bool GLASS_checkObjectType(GLuint obj, u32 type);

#endif /* _GLASS_TYPES_H */