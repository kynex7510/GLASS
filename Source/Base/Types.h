#ifndef _GLASS_BASE_TYPES_H
#define _GLASS_BASE_TYPES_H

#include <GLASS.h>

#include "Platform/GPUDefs.h"

#define DECL_FLAG(id) (u32)(1u << (id))

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

#define GLASS_SHADER_FLAG_DELETE DECL_FLAG(0)
#define GLASS_SHADER_FLAG_GEOMETRY DECL_FLAG(1)
#define GLASS_SHADER_FLAG_MERGE_OUTMAPS DECL_FLAG(2)
#define GLASS_SHADER_FLAG_USE_TEXCOORDS DECL_FLAG(3)

#define GLASS_PROGRAM_FLAG_DELETE DECL_FLAG(0)
#define GLASS_PROGRAM_FLAG_LINK_FAILED DECL_FLAG(1)
#define GLASS_PROGRAM_FLAG_UPDATE_VERTEX DECL_FLAG(2)
#define GLASS_PROGRAM_FLAG_UPDATE_GEOMETRY DECL_FLAG(3)

#define GLASS_ATTRIB_FLAG_ENABLED DECL_FLAG(0)
#define GLASS_ATTRIB_FLAG_FIXED DECL_FLAG(1)

#define GLASS_CONTEXT_FLAG_FRAMEBUFFER DECL_FLAG(0)
#define GLASS_CONTEXT_FLAG_DRAW DECL_FLAG(1)
#define GLASS_CONTEXT_FLAG_VIEWPORT DECL_FLAG(2)
#define GLASS_CONTEXT_FLAG_SCISSOR DECL_FLAG(3)
#define GLASS_CONTEXT_FLAG_ATTRIBS DECL_FLAG(4)
#define GLASS_CONTEXT_FLAG_PROGRAM DECL_FLAG(5)
#define GLASS_CONTEXT_FLAG_COMBINERS DECL_FLAG(6)
#define GLASS_CONTEXT_FLAG_FRAGOP DECL_FLAG(7)
#define GLASS_CONTEXT_FLAG_DEPTHMAP DECL_FLAG(8)
#define GLASS_CONTEXT_FLAG_COLOR_DEPTH DECL_FLAG(9)
#define GLASS_CONTEXT_FLAG_EARLY_DEPTH DECL_FLAG(10)
#define GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR DECL_FLAG(11)
#define GLASS_CONTEXT_FLAG_STENCIL DECL_FLAG(12)
#define GLASS_CONTEXT_FLAG_CULL_FACE DECL_FLAG(13)
#define GLASS_CONTEXT_FLAG_ALPHA DECL_FLAG(14)
#define GLASS_CONTEXT_FLAG_BLEND DECL_FLAG(15)
#define GLASS_CONTEXT_FLAG_TEXTURE DECL_FLAG(16)
#define GLASS_CONTEXT_FLAG_ALL (~(0u))

#define GLASS_OBJ_IS_BUFFER(x) GLASS_checkObjectType((x), GLASS_BUFFER_TYPE)
#define GLASS_OBJ_IS_RENDERBUFFER(x) GLASS_checkObjectType((x), GLASS_RENDERBUFFER_TYPE)
#define GLASS_OBJ_IS_FRAMEBUFFER(x) GLASS_checkObjectType((x), GLASS_FRAMEBUFFER_TYPE)

#define GLASS_OBJ_IS_PROGRAM(x) GLASS_checkObjectType((x), GLASS_PROGRAM_TYPE)
#define GLASS_OBJ_IS_SHADER(x) GLASS_checkObjectType((x), GLASS_SHADER_TYPE)

#define GLASS_OBJ_IS_TEXTURE(x) GLASS_checkObjectType((x), GLASS_TEXTURE_TYPE)

#define GLASS_OBJ(name) u32 _glObjectType

KYGX_INLINE bool GLASS_checkObjectType(GLuint obj, uint32_t type) {
    if (obj != GLASS_INVALID_OBJECT)
        return *(uint32_t*)obj == type;

    return false;
}

typedef struct {
    u32 glObjectType; // GL object type.
} GLObjectInfo;

typedef struct {
    GLASS_OBJ(GLASS_BUFFER_TYPE);
    u8* address;  // Data address.
    GLenum usage; // Buffer usage type.
    bool bound;   // If this buffer has been bound.
} BufferInfo;

typedef struct {
    GLASS_OBJ(GLASS_RENDERBUFFER_TYPE);
    u8* address;    // Data address.
    GLsizei width;  // Buffer width.
    GLsizei height; // Buffer height.
    GLenum format;  // Buffer format.
    bool bound;     // If this renderbuffer has been bound.
} RenderbufferInfo;

typedef struct {
    GLASS_OBJ(GLASS_FRAMEBUFFER_TYPE);
    GLuint colorBuffer; // Bound color buffer.
    GLuint depthBuffer; // Bound depth (+ stencil) buffer.
    size_t texFace;     // Texture object face.
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
    u32 flags;               // Program flags.
} ProgramInfo;

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

typedef struct {
    u8 ID;       // Constant ID.
    u32 data[3]; // Constant data.
} ConstFloatInfo;

typedef struct {
    GLASS_OBJ(GLASS_SHADER_TYPE);
    SharedShaderData* sharedData;       // Shared shader data.
    size_t codeEntrypoint;              // Code entrypoint.
    GPUGeoShaderMode gsMode;            // Mode for geometry shader.
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
    GLASS_OBJ(GLASS_TEXTURE_TYPE);
    GLenum target;                  // Texture target.
    GPUTexFormat format;            // Texture format.
    u16 width;                      // Texture width.
    u16 height;                     // Texture height.
    u8* faces[GLASS_NUM_TEX_FACES]; // Texture data.
    u32 borderColor;                // Border color (ABGR).
    GLenum minFilter;               // Minification filter.
    GLenum magFilter;               // Magnification filter.
    GLenum wrapS;                   // Wrap on the horizontal axis.
    GLenum wrapT;                   // Wrap on the vertical axis.
    float lodBias;                  // LOD bias.
    u8 minLod;                      // Min level of details.
    u8 maxLod;                      // Max level of details.
    bool vram;                      // Allocate data on VRAM.
} TextureInfo;

typedef struct {
    GLenum type;           // Type of each component.
    GLint count;           // Num of components.
    GLsizei stride;        // Buffer stride.
    GLuint boundBuffer;    // Bound array buffer.
    u32 physAddr;          // Physical address to component data.
    size_t bufferOffset;   // Offset to component data.
    size_t bufferSize;     // Buffer size (actual stride).
    GLfloat components[4]; // Fixed attrib X-Y-Z-W.
    size_t sizeOfPrePad;   // Size of padding preceeding component data.
    size_t sizeOfPostPad;  // Size of padding succeeding component data. 
    u16 flags;             // Attribute flags.
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
    u32 color;          // Constant color.
} CombinerInfo;

GLuint GLASS_createObject(u32 type);

#endif /* _GLASS_BASE_TYPES_H */