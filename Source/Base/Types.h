#ifndef _GLASS_BASE_TYPES_H
#define _GLASS_BASE_TYPES_H

#include <GLES/gl2.h>

#define GLASS_DECL_FLAG(id) (uint32_t)(1u << (id))

#define GLASS_INVALID_OBJECT 0
#define GLASS_TEX_TARGET_UNBOUND 0

#define GLASS_NUM_ATTRIB_REGS 16
#define GLASS_MAX_ENABLED_ATTRIBS 12
#define GLASS_NUM_BOOL_UNIFORMS 16
#define GLASS_NUM_INT_UNIFORMS 4
#define GLASS_NUM_FLOAT_UNIFORMS 96
#define GLASS_NUM_COMBINER_STAGES 6
#define GLASS_NUM_TEX_UNITS 3
#define GLASS_MAX_FB_WIDTH 1024
#define GLASS_MAX_FB_HEIGHT 1024
#define GLASS_MIN_TEX_SIZE 8
#define GLASS_MAX_TEX_SIZE 1024
#define GLASS_NUM_TEX_LEVELS 8
#define GLASS_NUM_TEX_FACES 6

#define GLASS_UNI_BOOL 0x00
#define GLASS_UNI_INT 0x01
#define GLASS_UNI_FLOAT 0x02

#define GLASS_BUFFER_TYPE 0x01
#define GLASS_RENDERBUFFER_TYPE 0x02
#define GLASS_FRAMEBUFFER_TYPE 0x03
#define GLASS_PROGRAM_TYPE 0x04
#define GLASS_SHADER_TYPE 0x05
#define GLASS_TEXTURE_TYPE 0x06

#define GLASS_SHADER_FLAG_DELETE GLASS_DECL_FLAG(0)
#define GLASS_SHADER_FLAG_GEOMETRY GLASS_DECL_FLAG(1)
#define GLASS_SHADER_FLAG_MERGE_OUTMAPS GLASS_DECL_FLAG(2)
#define GLASS_SHADER_FLAG_USE_TEXCOORDS GLASS_DECL_FLAG(3)

#define GLASS_PROGRAM_FLAG_DELETE GLASS_DECL_FLAG(0)
#define GLASS_PROGRAM_FLAG_LINK_FAILED GLASS_DECL_FLAG(1)
#define GLASS_PROGRAM_FLAG_UPDATE_VERTEX GLASS_DECL_FLAG(2)
#define GLASS_PROGRAM_FLAG_UPDATE_GEOMETRY GLASS_DECL_FLAG(3)

#define GLASS_ATTRIB_FLAG_ENABLED GLASS_DECL_FLAG(0)
#define GLASS_ATTRIB_FLAG_FIXED GLASS_DECL_FLAG(1)

#define GLASS_CONTEXT_FLAG_FRAMEBUFFER GLASS_DECL_FLAG(0)
#define GLASS_CONTEXT_FLAG_DRAW GLASS_DECL_FLAG(1)
#define GLASS_CONTEXT_FLAG_VIEWPORT GLASS_DECL_FLAG(2)
#define GLASS_CONTEXT_FLAG_SCISSOR GLASS_DECL_FLAG(3)
#define GLASS_CONTEXT_FLAG_ATTRIBS GLASS_DECL_FLAG(4)
#define GLASS_CONTEXT_FLAG_PROGRAM GLASS_DECL_FLAG(5)
#define GLASS_CONTEXT_FLAG_COMBINERS GLASS_DECL_FLAG(6)
#define GLASS_CONTEXT_FLAG_FRAGOP GLASS_DECL_FLAG(7)
#define GLASS_CONTEXT_FLAG_DEPTHMAP GLASS_DECL_FLAG(8)
#define GLASS_CONTEXT_FLAG_COLOR_DEPTH GLASS_DECL_FLAG(9)
#define GLASS_CONTEXT_FLAG_EARLY_DEPTH GLASS_DECL_FLAG(10)
#define GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR GLASS_DECL_FLAG(11)
#define GLASS_CONTEXT_FLAG_STENCIL GLASS_DECL_FLAG(12)
#define GLASS_CONTEXT_FLAG_CULL_FACE GLASS_DECL_FLAG(13)
#define GLASS_CONTEXT_FLAG_ALPHA GLASS_DECL_FLAG(14)
#define GLASS_CONTEXT_FLAG_BLEND GLASS_DECL_FLAG(15)
#define GLASS_CONTEXT_FLAG_TEXTURE GLASS_DECL_FLAG(16)
#define GLASS_CONTEXT_FLAG_ALL (~(0u))

#define GLASS_OBJ_IS_BUFFER(x) GLASS_checkObjectType((x), GLASS_BUFFER_TYPE)
#define GLASS_OBJ_IS_RENDERBUFFER(x) GLASS_checkObjectType((x), GLASS_RENDERBUFFER_TYPE)
#define GLASS_OBJ_IS_FRAMEBUFFER(x) GLASS_checkObjectType((x), GLASS_FRAMEBUFFER_TYPE)

#define GLASS_OBJ_IS_PROGRAM(x) GLASS_checkObjectType((x), GLASS_PROGRAM_TYPE)
#define GLASS_OBJ_IS_SHADER(x) GLASS_checkObjectType((x), GLASS_SHADER_TYPE)

#define GLASS_OBJ_IS_TEXTURE(x) GLASS_checkObjectType((x), GLASS_TEXTURE_TYPE)

#define GLASS_OBJ(name) uint32_t _glObjectType

GLuint GLASS_createObject(uint32_t type);

inline bool GLASS_checkObjectType(GLuint obj, uint32_t type) {
    if (obj != GLASS_INVALID_OBJECT)
        return *(uint32_t*)obj == type;

    return false;
}

typedef struct {
    uint32_t glObjectType; // GL object type.
} GLObjectInfo;

typedef struct {
    GLASS_OBJ(GLASS_BUFFER_TYPE);
    uint8_t* address; // Data address.
    GLenum usage;     // Buffer usage type.
    bool bound;       // If this buffer has been bound.
} BufferInfo;

typedef struct {
    GLASS_OBJ(GLASS_RENDERBUFFER_TYPE);
    uint8_t* address; // Data address.
    GLsizei width;    // Buffer width.
    GLsizei height;   // Buffer height.
    GLenum format;    // Buffer format.
    bool bound;       // If this renderbuffer has been bound.
} RenderbufferInfo;

typedef struct {
    GLASS_OBJ(GLASS_FRAMEBUFFER_TYPE);
    GLuint colorBuffer; // Bound color buffer.
    GLuint depthBuffer; // Bound depth (+ stencil) buffer.
    bool bound;         // If this framebuffer has been bound.
} FramebufferInfo;

typedef struct {
    GLASS_OBJ(GLASS_PROGRAM_TYPE);
    // Attached = result of glAttachShader.
    // Linked = result of glLinkProgram.
    GLuint attachedVertex;   // Attached vertex shader.
    GLuint linkedVertex;     // Linked vertex shader.
    GLuint attachedGeometry; // Attached geometry shader.
    GLuint linkedGeometry;   // Linked geometry shader.
    uint32_t flags;          // Program flags.
} ProgramInfo;

typedef struct {
    uint32_t refc;           // Reference count.
    uint32_t* binaryCode;    // Binary code buffer.
    uint32_t numOfCodeWords; // Num of instructions.
    uint32_t* opDescs;       // Operand descriptors.
    uint32_t numOfOpDescs;   // Num of operand descriptors.
} SharedShaderData;

typedef struct {
    uint8_t ID;      // Uniform ID.
    uint8_t type;    // Uniform type.
    size_t count;    // Number of elements.
    char* symbol;    // Pointer to symbol.
    union {
        uint16_t mask;    // Bool, bool array mask.
        uint32_t value;   // Int value.
        uint32_t* values; // int array, float, float array data.
    } data;
    bool dirty; // Uniform dirty.
} UniformInfo;

typedef struct {
    uint8_t ID;   // Attribute ID.
    char* symbol; // Pointer to symbol.
} ActiveAttribInfo;

typedef struct {
    uint8_t ID;       // Constant ID.
    uint32_t data[3]; // Constant data.
} ConstFloatInfo;

typedef struct {
    GLASS_OBJ(GLASS_SHADER_TYPE);
    SharedShaderData* sharedData;       // Shared shader data.
    size_t codeEntrypoint;              // Code entrypoint.
    uint16_t outMask;                   // Used output registers mask.
    uint16_t outTotal;                  // Total number of output registers.
    uint32_t outSems[7];                // Output register semantics.
    uint32_t outClock;                  // Output register clock.
    char* symbolTable;                  // This shader symbol table.
    size_t sizeOfSymbolTable;           // Size of symbol table.
    uint8_t gsMode;                     // Mode for geometry shader.
    uint16_t constBoolMask;             // Constant bool uniform mask.
    uint32_t constIntData[4];           // Constant int uniform data.
    uint16_t constIntMask;              // Constant int uniform mask.
    ConstFloatInfo* constFloatUniforms; // Constant uniforms.
    size_t numOfConstFloatUniforms;     // Num of const uniforms.
    UniformInfo* activeUniforms;        // Active uniforms.
    size_t numOfActiveUniforms;         // Num of active uniforms.
    size_t activeUniformsMaxLen;        // Max length for active uniform symbols.
    ActiveAttribInfo* activeAttribs;    // Active attributes.
    size_t numOfActiveAttribs;          // Num of active attributes.
    size_t activeAttribsMaxLen;         // Max length for active attribute symbols.
    uint16_t flags;                     // Shader flags.
    uint16_t refc;                      // Reference count.
} ShaderInfo;

typedef struct {
    GLASS_OBJ(GLASS_TEXTURE_TYPE);
    GLenum target;                       // Texture target.
    glassPixelFormat pixelFormat;        // Texture pixel format.
    uint16_t width;                      // Texture width.
    uint16_t height;                     // Texture height.
    uint8_t* faces[GLASS_NUM_TEX_FACES]; // Texture data.
    uint32_t borderColor;                // Border color (ABGR).
    GLenum minFilter;                    // Minification filter.
    GLenum magFilter;                    // Magnification filter.
    GLenum wrapS;                        // Wrap on the horizontal axis.
    GLenum wrapT;                        // Wrap on the vertical axis.
    float lodBias;                       // LOD bias.
    uint8_t minLod;                      // Min level of details.
    uint8_t maxLod;                      // Max level of details.
    bool vram;                           // Allocate data on VRAM.
} TextureInfo;

typedef struct {
    GLenum type;           // Type of each component.
    GLint count;           // Num of components.
    GLsizei stride;        // Buffer stride.
    GLuint boundBuffer;    // Bound array buffer.
    uint32_t physAddr;     // Physical address to component data.
    size_t bufferOffset;   // Offset to component data.
    size_t bufferSize;     // Buffer size (actual stride).
    GLfloat components[4]; // Fixed attrib X-Y-Z-W.
    size_t sizeOfPrePad;   // Size of padding preceeding component data.
    size_t sizeOfPostPad;  // Size of padding succeeding component data. 
    uint16_t flags;        // Attribute flags.
} AttributeInfo;

typedef struct {
    GLenum rgbSrc[3];   // RGB source 0-1-2.
    GLenum alphaSrc[3]; // Alpha source 0-1-2.
    GLenum rgbOp[3];    // RGB operand 0-1-2.
    GLenum alphaOp[3];  // Alpha operand 0-1-2.
    GLenum rgbFunc;     // RGB function.
    GLenum alphaFunc;   // Alpha function.
    GLfloat rgbScale;   // RGB scale.
    GLfloat alphaScale; // Alpha scale.
    uint32_t color;     // Constant color.
} CombinerInfo;

typedef struct {
    glassInitParams initParams; // Context parameters.
    glassSettings settings;     // Context settings.

    /* Platform */
    uint32_t flags;         // State flags.
    GLenum lastError;       // Actually first error.
    // gxCmdQueue_s gxQueue;   // Queue for GX commands.

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

GLuint GLASS_createObject(uint32_t type);
bool GLASS_checkObjectType(GLuint obj, uint32_t type);

#endif /* _GLASS_BASE_TYPES_H */