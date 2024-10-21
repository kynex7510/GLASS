#ifndef _GLASS_UTILITY_H
#define _GLASS_UTILITY_H

#include "Types.h"

#define STRINGIFY(x) #x
#define AS_STRING(x) STRINGIFY(x)

#define NORETURN __attribute__((noreturn))
#define WEAK __attribute__((weak))
#define INLINE inline __attribute__((always_inline))

#ifndef NDEBUG

#define LOG(msg) GLASS_utility_logImpl((msg), sizeof((msg)) - 1)

#define ASSERT(cond)                                                                                  \
    do {                                                                                              \
        if (!(cond)) {                                                                                \
            LOG("Assertion failed: " #cond "\nIn file: " __FILE__ "\nOn line: " AS_STRING(__LINE__)); \
            GLASS_utility_abort();                                                                    \
        }                                                                                             \
    } while (false)

#define UNREACHABLE(msg)                                                                         \
    do {                                                                                         \
        LOG("Unreachable point: " msg "\nIn file: " __FILE__ "\nOn line: " AS_STRING(__LINE__)); \
        GLASS_utility_abort();                                                                   \
    } while (false)

void GLASS_utility_logImpl(const char* msg, size_t len);

#else

#define LOG(msg)
#define ASSERT(cond) (void)(cond)
#define UNREACHABLE(msg) GLASS_utility_abort()

#endif // NDEBUG

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, max, val) (MAX((min), MIN((max), (val))))

typedef enum {
    Emu_None = 0,
    Emu_Citra = 1,
    Emu_Panda = 2,
} Emulator;

Emulator GLASS_utility_detectEmulator(void);
INLINE bool GLASS_utility_isEmulator(void) { return GLASS_utility_detectEmulator() != Emu_None; }

void GLASS_utility_abort(void) NORETURN;

void* GLASS_utility_convertPhysToVirt(u32 addr);
float GLASS_utility_f24tof32(u32 f);
u32 GLASS_utility_f32tofixed13(float f);

u32 GLASS_utility_makeClearColor(GLenum format, u32 color);
u32 GLASS_utility_makeClearDepth(GLenum format, GLclampf factor, u8 stencil);
size_t GLASS_utility_getRBBytesPerPixel(GLenum format);
size_t GLASS_utility_getPixelSizeForFB(GLenum format);

GLenum GLASS_utility_wrapFBFormat(GSPGPU_FramebufferFormat format);

GX_TRANSFER_FORMAT GLASS_utility_getTransferFormat(GLenum format);
GPU_COLORBUF GLASS_utility_getRBFormat(GLenum format);
GPU_FORMATS GLASS_utility_getAttribType(GLenum type);
GPU_TESTFUNC GLASS_utility_getTestFunc(GLenum func);
GPU_EARLYDEPTHFUNC GLASS_utility_getEarlyDepthFunc(GLenum func);
GPU_STENCILOP GLASS_utility_getStencilOp(GLenum op);
GPU_BLENDEQUATION GLASS_utility_getBlendEq(GLenum eq);
GPU_BLENDFACTOR GLASS_utility_getBlendFactor(GLenum func);
GPU_LOGICOP GLASS_utility_getLogicOp(GLenum op);
GPU_TEVSRC GLASS_utility_getCombinerSrc(GLenum src);
GPU_TEVOP_RGB GLASS_utility_getCombinerOpRGB(GLenum op);
GPU_TEVOP_A GLASS_utility_getCombinerOpAlpha(GLenum op);
GPU_COMBINEFUNC GLASS_utility_getCombinerFunc(GLenum combiner);
GPU_TEVSCALE GLASS_utility_getCombinerScale(GLfloat scale);
GPU_Primitive_t GLASS_utility_getDrawPrimitive(GLenum mode);
u32 GLASS_utility_getDrawType(GLenum type);
u32 GLASS_utility_makeTransferFlags(bool flipVertical, bool tilted, bool rawCopy, GX_TRANSFER_FORMAT inputFormat, GX_TRANSFER_FORMAT outputFormat, GX_TRANSFER_SCALE scaling);

void GLASS_utility_packIntVector(const u32* in, u32* out);
void GLASS_utility_unpackIntVector(u32 in, u32* out);

void GLASS_utility_packFloatVector(const float* in, u32* out);
void GLASS_utility_unpackFloatVector(const u32* in, float* out);

bool GLASS_utility_getBoolUniform(const UniformInfo* info, size_t offset);
void GLASS_utility_setBoolUniform(UniformInfo* info, size_t offset, bool enabled);

void GLASS_utility_getIntUniform(const UniformInfo* info, size_t offset, u32* out);
void GLASS_utility_setIntUniform(UniformInfo* info, size_t offset, u32 vector);

void GLASS_utility_getFloatUniform(const UniformInfo* info, size_t offset, u32* out);
void GLASS_utility_setFloatUniform(UniformInfo* info, size_t offset, const u32* vectorData);

GPU_TEXCOLOR GLASS_utility_getTexFormat(GLenum format, GLenum dataType);
GLenum GLASS_utility_getTexDataType(GPU_TEXCOLOR format);
bool GLASS_utility_isValidTexCombination(GLenum format, GLenum dataType);
GLenum GLASS_utility_wrapTexFormat(GPU_TEXCOLOR format);
size_t GLASS_utility_getTexBitsPerPixel(GPU_TEXCOLOR format);
size_t GLASS_utility_calculateTexSize(u16 width, u16 height, GPU_TEXCOLOR format);
GPU_TEXTURE_FILTER_PARAM GLASS_utility_getTexFilter(GLenum filter);
GPU_TEXTURE_FILTER_PARAM GLASS_utility_getMipFilter(GLenum minFilter);
GPU_TEXTURE_WRAP_PARAM GLASS_utility_getTexWrap(GLenum wrap);

#endif /* _GLASS_UTILITY_H */