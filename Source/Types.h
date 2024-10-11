#ifndef _GLASS_TYPES_H
#define _GLASS_TYPES_H

#include "GLASS.h"
#include "GLASS/Defs.h"
#include "Constants.h"

#include <stddef.h>

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
    u32 type;         // GL type (GLASS_TEXTURE_TYPE).
    GLenum target;    // Texture target.
    u32 borderColor;  // Border color (ABGR).
    u16 width;        // Texture width.
    u16 height;       // Texture height.
    GLenum minFilter; // Min filter.
    GLenum magFilter; // Mag filter.
    GLenum wrapS;     // Wrap S.
    GLenum wrapT;     // Wrap T.
    u8 minLod;        // Min level of details.
    u8 maxLod;        // Max level of details.
    u16 flags;        // Texture flags.
    float lodBias;    // LOD bias.
} TextureInfo;

typedef struct {
    GLuint texture; // Bound texture.
    bool dirty;     // Needs update.
} TextureUnit;

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
    GLsizei stride;        // Buffer stride.
    GLuint boundBuffer;    // Bound array buffer.
    u32 physAddr;          // Buffer physical address.
    u32 physOffset;        // Physical offset to buffer.
    GLfloat components[4]; // Fixed attrib X-Y-Z-W.
    u16 flags;             // Attribute flags.
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

typedef struct {
    u8 ID;        // Attribute ID.
    char* symbol; // Pointer to symbol.
} ActiveAttribInfo;

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
    size_t sizeOfSymbolTable;           // Size of symbol table.
    u16 constBoolMask;                  // Constant bool uniform mask.
    u32 constIntData[4];                // Constant int uniform data.
    u16 constIntMask;                   // Constant int uniform mask.
    ConstFloatInfo* constFloatUniforms; // Constant uniforms.
    size_t numOfConstFloatUniforms;     // Num of const uniforms.
    UniformInfo* activeUniforms;        // Active uniforms.
    size_t numOfActiveUniforms;         // Num of active uniforms.
    size_t activeUniformsMaxLen;        // Max length for active uniform symbols.
    ActiveAttribInfo* activeAttribs;    // Active attributes.
    size_t numOfActiveAttribs;          // Num of active attributes.
    size_t activeAttribsMaxLen;         // Max length for active attribute symbols.
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
    glassInitParams initParams; // Context parameters.
    glassSettings settings;     // Context settings.

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

    /* Texture */
    TextureUnit textureUnits[GLASS_NUM_TEXTURE_UNITS]; // Texture units.
    size_t activeTextureUnit;                          // Currently active texture unit.    

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
    GLsizei numEnabledAttribs;                    // Number of enabled attributes.

    /* Combiners */
    GLint combinerStage;                               // Current combiner stage.
    CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES]; // Combiners.

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
    u32 blendColor;       // Blend color.
    GLenum blendEqRGB;    // Blend equation RGB.
    GLenum blendEqAlpha;  // Blend equation alpha.
    GLenum blendSrcRGB;   // Blend source RGB.
    GLenum blendDstRGB;   // Blend destination RGB.
    GLenum blendSrcAlpha; // Blend source alpha.
    GLenum blendDstAlpha; // Blend destination alpha.
    GLenum logicOp;       // Logic op.
} CtxCommon;

GLuint GLASS_createObject(u32 type);
bool GLASS_checkObjectType(GLuint obj, u32 type);

#endif /* _GLASS_TYPES_H */