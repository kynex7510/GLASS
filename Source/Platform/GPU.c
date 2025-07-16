/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <KYGX/Utility.h>

#include "Platform/GPU.h"
#include "Base/Math.h"
#include "Base/TexManager.h"

#include <string.h> // memcpy, memset

#ifdef KYGX_BAREMETAL
#include <mem_map.h> // VRAM_BASE
#include <arm11/drivers/gpu_regs.h>
#define PHYSICAL_BUFFER_BASE VRAM_BASE
#else
#define PHYSICAL_BUFFER_BASE OS_VRAM_PADDR
#endif // KYGX_BAREMETAL

#define DEFAULT_CMDBUF_CAPACITY 0x4000

#define PAD_4 12
#define PAD_8 13
#define PAD_12 14
#define PAD_16 15

#define CMD_HEADER(id, mask, numParams, consecutive) \
    (((id) & 0xFFFF) | (((mask) & 0xF) << 16) | ((((numParams) - 1) & 0xFF) << 20) | ((consecutive) ? (1 << 31) : 0))

static inline size_t addCmdImplStep(u32* cmdBuffer, u32 header, const u32* params, size_t numParams) {
    KYGX_ASSERT(cmdBuffer);
    KYGX_ASSERT(params);
    KYGX_ASSERT(numParams);
    KYGX_ASSERT(kygxIsAligned((size_t)cmdBuffer, 8));

    // Write header + first parameter.
    cmdBuffer[0] = params[0];
    cmdBuffer[1] = header;

    // Write other parameters.
    memcpy(&cmdBuffer[2], &params[1], (numParams - 1) * sizeof(u32));

    // Make sure commands are kept aligned.
    if ((numParams + 1) & 1) {
        ++numParams;
        cmdBuffer[numParams] = 0;
    }

    return (numParams + 1) * sizeof(u32);
}

static void addMultiParamCmd(GLASSGPUCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams, bool consecutive) {
    KYGX_ASSERT(list);
    KYGX_ASSERT(params);
    KYGX_ASSERT(numParams > 0);
    
    if (list->offset + (numParams * sizeof(u32)) >= list->capacity) {
        KYGX_UNREACHABLE("GPU command list OOB!");
    }

    for (size_t i = 0; i < numParams; i += 256) {
        u32* cmdBuffer = (u32*)((u8*)(list->mainBuffer) + list->offset);

        // Calculate current number of parameters and header.
        const size_t curNumParams = GLASS_MIN(numParams, 255);
        const u32 header = CMD_HEADER(id, mask, curNumParams, consecutive);

        // Write params data.
        list->offset += addCmdImplStep(cmdBuffer, header, &params[i], curNumParams);

        // Update id for consecutive writes.
        if (consecutive)
            id += curNumParams;
    }
}

static inline void addMaskedWrites(GLASSGPUCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams) {
    addMultiParamCmd(list, id, mask, params, numParams, false);
}

static inline void addMaskedIncrementalWrites(GLASSGPUCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams) {
    addMultiParamCmd(list, id, mask, params, numParams, true);
}

static inline void addWrites(GLASSGPUCommandList* list, u32 id, const u32* params, size_t numParams) { addMaskedWrites(list, id, 0xF, params, numParams); };

static inline void addIncrementalWrites(GLASSGPUCommandList* list, u32 id, const u32* params, size_t numParams) {
    addMaskedIncrementalWrites(list, id, 0xF, params, numParams);
}

static inline void addMaskedWrite(GLASSGPUCommandList* list, u32 id, u32 mask, u32 v) {
    KYGX_ASSERT(list);
    KYGX_ASSERT(list->offset + (2 * sizeof(u32)) < list->capacity);
    u32* cmdBuffer = (u32*)((u8*)(list->mainBuffer) + list->offset);
    cmdBuffer[0] = v;
    cmdBuffer[1] = CMD_HEADER(id, mask, 1, false);
    list->offset += 2 * sizeof(u32);
}

static inline void addWrite(GLASSGPUCommandList* list, u32 id, u32 v) { addMaskedWrite(list, id, 0xF, v); }

void GLASS_gpu_allocList(GLASSGPUCommandList* list) {
    KYGX_ASSERT(list);

    if (!list->capacity) {
        KYGX_ASSERT(!list->mainBuffer);
        KYGX_ASSERT(!list->secondBuffer);
        list->capacity = DEFAULT_CMDBUF_CAPACITY;
    }

    KYGX_ASSERT(kygxIsAligned(list->capacity, 16));

    if (!list->mainBuffer) {
        list->mainBuffer = glassLinearAlloc(list->capacity);
        KYGX_ASSERT(list->mainBuffer);
    }

    KYGX_ASSERT(glassIsLinear(list->mainBuffer));

    if (!list->secondBuffer) {
        list->secondBuffer = glassLinearAlloc(list->capacity);
        KYGX_ASSERT(list->secondBuffer);
    }

    KYGX_ASSERT(glassIsLinear(list->secondBuffer));
}

void GLASS_gpu_freeList(GLASSGPUCommandList* list) {
    KYGX_ASSERT(list);

    glassLinearFree(list->secondBuffer);
    glassLinearFree(list->mainBuffer);
    list->secondBuffer = NULL;
    list->mainBuffer = NULL;
    list->capacity = 0;
    list->offset = 0;
}

bool GLASS_gpu_swapListBuffers(GLASSGPUCommandList* list, void** outBuffer, size_t* outSize) {
    KYGX_ASSERT(list);
    KYGX_ASSERT(outBuffer);
    KYGX_ASSERT(outSize);

    if (list->offset > 0) {
        // Finalize list.
        addWrite(list, GPUREG_FINALIZE, 0x12345678);
        if (!kygxIsAligned(list->offset, 16))
            addWrite(list, 0, 0x75107510);

        if (outBuffer)
            *outBuffer = list->mainBuffer;

        if (outSize)
            *outSize = list->offset;

        void* tmp = list->mainBuffer;
        list->mainBuffer = list->secondBuffer;
        list->secondBuffer = tmp;
        list->offset = 0;
        return true;
    }

    return false;
}

static inline size_t unwrapRBPixelSize(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return 2;
        case GL_RGB8_OES:
            return 1;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
            return 0;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPURenderbufferFormat unwrapRBFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return RBFMT_RGBA8;
        case GL_RGB8_OES:
            return RBFMT_RGB8;
        case GL_RGB5_A1:
            return RBFMT_RGBA5551;
        case GL_RGB565:
            return RBFMT_RGB565;
        case GL_RGBA4:
            return RBFMT_RGBA4;
        case GL_DEPTH_COMPONENT16:
            return RBFMT_DEPTH16;
        case GL_DEPTH_COMPONENT24_OES:
            return RBFMT_DEPTH24;
        case GL_DEPTH24_STENCIL8_OES:
            return RBFMT_DEPTH24_STENCIL8;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_bindFramebuffer(GLASSGPUCommandList* list, const FramebufferInfo* info, bool block32) {
    u8* colorBuffer = NULL;
    u8* depthBuffer = NULL;
    u32 width = 0;
    u32 height = 0;
    GLenum colorFormat = 0;
    GLenum depthFormat = 0;
    u32 params[4];

    if (info) {
        const RenderbufferInfo* db = (RenderbufferInfo*)info->depthBuffer;
        if (db) {
            depthBuffer = db->address;
            width = db->width;
            height = db->height;
            depthFormat = db->format;
        }

        if (GLASS_OBJ_IS_RENDERBUFFER(info->colorBuffer)) {
            const RenderbufferInfo* cb = (RenderbufferInfo*)info->colorBuffer;
            colorBuffer = cb->address;
            width = cb->width;
            height = cb->height;
            colorFormat = cb->format;
        } else if (GLASS_OBJ_IS_TEXTURE(info->colorBuffer)) {
            RenderbufferInfo cb;
            GLASS_tex_getAsRenderbuffer((const TextureInfo*)info->colorBuffer, info->texFace, &cb);
            colorBuffer = cb.address;
            width = cb.width;
            height = cb.height;
            colorFormat = cb.format;
        }
    }

    GLASS_gpu_invalidateFramebuffer(list);

    // Set depth buffer, color buffer and dimensions.
    params[0] = kygxGetPhysicalAddress(depthBuffer) >> 3;
    params[1] = kygxGetPhysicalAddress(colorBuffer) >> 3;
    params[2] = 0x01000000 | (((width - 1) & 0xFFF) << 12) | (height & 0xFFF);
    addIncrementalWrites(list, GPUREG_DEPTHBUFFER_LOC, params, 3);
    addWrite(list, GPUREG_RENDERBUF_DIM, params[2]);

    // Set buffer parameters.
    if (colorBuffer) {
        addWrite(list, GPUREG_COLORBUFFER_FORMAT, (unwrapRBFormat(colorFormat) << 16) | unwrapRBPixelSize(colorFormat));
        params[0] = params[1] = 0x0F;
    } else {
        params[0] = params[1] = 0;
    }

    if (depthBuffer) {
        addWrite(list, GPUREG_DEPTHBUFFER_FORMAT, unwrapRBFormat(depthFormat));
        params[2] = params[3] = depthFormat == GL_DEPTH24_STENCIL8_OES ? 0x03 : 0x02;
    } else {
        params[2] = params[3] = 0;
    }

    addWrite(list, GPUREG_FRAMEBUFFER_BLOCK32, block32 ? 1 : 0);
    addIncrementalWrites(list, GPUREG_COLORBUFFER_READ, params, 4);
}

void GLASS_gpu_flushFramebuffer(GLASSGPUCommandList* list) { addWrite(list, GPUREG_FRAMEBUFFER_FLUSH, 1); }
void GLASS_gpu_invalidateFramebuffer(GLASSGPUCommandList* list) { addWrite(list, GPUREG_FRAMEBUFFER_INVALIDATE, 1); }

void GLASS_gpu_setViewport(GLASSGPUCommandList* list, GLint x, GLint y, GLsizei width, GLsizei height) {
    u32 data[4];

    data[0] = GLASS_math_f32tof24(height / 2.0f);
    data[1] = GLASS_math_f32tof31(2.0f / height) << 1;
    data[2] = GLASS_math_f32tof24(width / 2.0f);
    data[3] = GLASS_math_f32tof31(2.0f / width) << 1;

    addIncrementalWrites(list, GPUREG_VIEWPORT_WIDTH, data, 4);
    addWrite(list, GPUREG_VIEWPORT_XY, (x << 16) | (y & 0xFFFF));
}

void GLASS_gpu_setScissorTest(GLASSGPUCommandList* list, GPUScissorMode mode, GLint x, GLint y, GLsizei width, GLsizei height) {
    addMaskedWrite(list, GPUREG_SCISSORTEST_MODE, 0x01, mode);
    addWrite(list, GPUREG_SCISSORTEST_POS, (y << 16) | (x & 0xFFFF));
    addWrite(list, GPUREG_SCISSORTEST_DIM, ((width - x - 1) << 16) | ((height - y - 1) & 0xFFFF));
}

static void uploadShaderBinary(GLASSGPUCommandList* list, const ShaderInfo* shader) {
    if (shader->sharedData) {
        // Set write offset for code upload.
        addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_CONFIG : GPUREG_VSH_CODETRANSFER_CONFIG, 0);

        // Write code.
        addWrites(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_DATA : GPUREG_VSH_CODETRANSFER_DATA,
            shader->sharedData->binaryCode,
            shader->sharedData->numOfCodeWords < 512 ? shader->sharedData->numOfCodeWords : 512);

        // Finalize code.
        addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_END : GPUREG_VSH_CODETRANSFER_END, 1);

        // Set write offset for op descs.
        addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_CONFIG : GPUREG_VSH_OPDESCS_CONFIG, 0);

        // Write op descs.
        addWrites(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_DATA : GPUREG_VSH_OPDESCS_DATA,
            shader->sharedData->opDescs,
            shader->sharedData->numOfOpDescs < 128 ? shader->sharedData->numOfOpDescs : 128);
    }
}

void GLASS_gpu_bindShaders(GLASSGPUCommandList* list, const ShaderInfo* vertexShader, const ShaderInfo* geometryShader) {
    // Initialize geometry engine.
    addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 0x03, geometryShader ? 2 : 0);
    addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 0x03, 0);
    addMaskedWrite(list, GPUREG_VSH_COM_MODE, 0x01, geometryShader ? 1 : 0);

    if (vertexShader) {
        uploadShaderBinary(list, vertexShader);
        addWrite(list, GPUREG_VSH_ENTRYPOINT, 0x7FFF0000 | (vertexShader->codeEntrypoint & 0xFFFF));
        addMaskedWrite(list, GPUREG_VSH_OUTMAP_MASK, 0x03, vertexShader->outMask);

        // Set vertex shader outmap number.
        addMaskedWrite(list, GPUREG_VSH_OUTMAP_TOTAL1, 0x01, vertexShader->outTotal - 1);
        addMaskedWrite(list, GPUREG_VSH_OUTMAP_TOTAL2, 0x01, vertexShader->outTotal - 1);
    }

    if (geometryShader) {
        uploadShaderBinary(list, geometryShader);
        addWrite(list, GPUREG_GSH_ENTRYPOINT, 0x7FFF0000 | (geometryShader->codeEntrypoint & 0xFFFF));
        addMaskedWrite(list, GPUREG_GSH_OUTMAP_MASK, 0x01, geometryShader->outMask);
    }

    // Handle outmaps.
    u16 mergedOutTotal = 0;
    u32 mergedOutClock = 0;
    u32 mergedOutSems[7];
    bool useTexcoords = false;

    if (vertexShader && geometryShader && (geometryShader->flags & GLASS_SHADER_FLAG_MERGE_OUTMAPS)) {
        // Merge outmaps.
        for (size_t i = 0; i < 7; ++i) {
            const u32 vshOutSem = vertexShader->outSems[i];
            u32 gshOutSem = geometryShader->outSems[i];

            if (vshOutSem != gshOutSem) {
                gshOutSem = gshOutSem != 0x1F1F1F1F ? gshOutSem : vshOutSem;
        }

        mergedOutSems[i] = gshOutSem;

        if (gshOutSem != 0x1F1F1F1F)
            ++mergedOutTotal;
        }

        mergedOutClock = vertexShader->outClock | geometryShader->outClock;
        useTexcoords = (vertexShader->flags & GLASS_SHADER_FLAG_USE_TEXCOORDS) | (geometryShader->flags & GLASS_SHADER_FLAG_USE_TEXCOORDS);
    } else {
        const ShaderInfo* mainShader = geometryShader ? geometryShader : vertexShader;
        if (mainShader) {
            mergedOutTotal = mainShader->outTotal;
            memcpy(mergedOutSems, mainShader->outSems, sizeof(mainShader->outSems));
            mergedOutClock = mainShader->outClock;
            useTexcoords = mainShader->flags & GLASS_SHADER_FLAG_USE_TEXCOORDS;
        }
    }

    if (mergedOutTotal) {
        addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x01, mergedOutTotal - 1);
        addMaskedWrite(list, GPUREG_SH_OUTMAP_TOTAL, 0x01, mergedOutTotal);
        addIncrementalWrites(list, GPUREG_SH_OUTMAP_O0, mergedOutSems, 7);
        addMaskedWrite(list, GPUREG_SH_OUTATTR_MODE, 0x01, useTexcoords ? 1 : 0);
        addWrite(list, GPUREG_SH_OUTATTR_CLOCK, mergedOutClock);
    }

    // TODO: configure geostage.
    addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 0x0A, 0);
    addWrite(list, GPUREG_GSH_MISC0, 0);
    addWrite(list, GPUREG_GSH_MISC1, 0);
    addWrite(list, GPUREG_GSH_INPUTBUFFER_CONFIG, 0xA0000000);
}

static inline void uploadBoolUniformMask(GLASSGPUCommandList* list, const ShaderInfo* shader, u16 mask) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_BOOLUNIFORM : GPUREG_VSH_BOOLUNIFORM;
    addWrite(list, reg, 0x7FFF0000 | mask);
}

static inline void uploadConstIntUniforms(GLASSGPUCommandList* list ,const ShaderInfo* shader) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    for (size_t i = 0; i < 4; ++i) {
        if (!((shader->constIntMask >> i) & 1))
            continue;

        addWrite(list, reg + i, shader->constIntData[i]);
    }
}

static inline void uploadConstFloatUniforms(GLASSGPUCommandList* list, const ShaderInfo* shader) {
    const u32 idReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    for (size_t i = 0; i < shader->numOfConstFloatUniforms; ++i) {
        const ConstFloatInfo* uni = &shader->constFloatUniforms[i];
        addWrite(list, idReg, uni->ID);
        addIncrementalWrites(list, dataReg, uni->data, 3);
    }
}

void GLASS_gpu_uploadConstUniforms(GLASSGPUCommandList* list, const ShaderInfo* shader) {
    KYGX_ASSERT(shader);
    uploadBoolUniformMask(list, shader, shader->constBoolMask);
    uploadConstIntUniforms(list, shader);
    uploadConstFloatUniforms(list, shader);
}

static inline void uploadIntUniform(GLASSGPUCommandList* list, const ShaderInfo* shader, UniformInfo* info) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    if (info->count == 1) {
        addWrite(list, reg + info->ID, info->data.value);
    } else {
        addIncrementalWrites(list, reg + info->ID, info->data.values, info->count);
    }
}

static inline void uploadFloatUniform(GLASSGPUCommandList* list, const ShaderInfo* shader, UniformInfo* info) {
    const u32 idReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    // ID is automatically incremented after each write.
    addWrite(list, idReg, info->ID);

    for (size_t i = 0; i < info->count; ++i)
        addIncrementalWrites(list, dataReg, &info->data.values[i * 3], 3);
}

void GLASS_gpu_uploadUniforms(GLASSGPUCommandList* list, ShaderInfo* shader) {
    KYGX_ASSERT(shader);

    bool uploadBool = false;
    u16 boolMask = shader->constBoolMask;

    for (size_t i = 0; i < shader->numOfActiveUniforms; ++i) {
        UniformInfo* uni = &shader->activeUniforms[i];

        if (!uni->dirty)
            continue;

        switch (uni->type) {
            case GLASS_UNI_BOOL:
                boolMask |= uni->data.mask;
                uploadBool = true;
                break;
            case GLASS_UNI_INT:
                uploadIntUniform(list, shader, uni);
                break;
            case GLASS_UNI_FLOAT:
                uploadFloatUniform(list, shader, uni);
                break;
            default:
                KYGX_UNREACHABLE("Invalid uniform type!");
        }

        uni->dirty = false;
    }

    if (uploadBool)
        uploadBoolUniformMask(list, shader, boolMask);
}

static GPUAttribType unwrapAttribType(GLenum type) {
    switch (type) {
        case GL_BYTE:
            return ATTRIBTYPE_BYTE;
        case GL_UNSIGNED_BYTE:
            return ATTRIBTYPE_UNSIGNED_BYTE;
        case GL_SHORT:
            return ATTRIBTYPE_SHORT;
        case GL_FLOAT:
            return ATTRIBTYPE_FLOAT;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static size_t insertAttribPad(u32* permutation, size_t startIndex, size_t padSize) {
    KYGX_ASSERT(permutation);

    size_t index = startIndex;
    size_t handled = 0;
    while (handled < padSize) {
        const size_t diff = padSize - handled;
        u32 padValue = 0;
        if (diff >= 16) {
            padValue = PAD_16;
            handled += 16;
        } else if (diff >= 12) {
            padValue = PAD_12;
            handled += 12;
        } else if (diff >= 8) {
            padValue = PAD_8;
            handled += 8;
        } else {
            padValue = PAD_4;
            handled += 4;
        }

        if (index < 8) {
            permutation[0] |= (padValue << (index * 4));
        } else {
            permutation[1] |= (padValue << (index * 4));
        }

        ++index;
        KYGX_ASSERT(index < 12); // We can have at most 12 components.
    }

    return index;
}

void GLASS_gpu_uploadAttributes(GLASSGPUCommandList* list, const AttributeInfo* attribs) {
    KYGX_ASSERT(attribs);

    u32 format[2];
    u32 permutation[2];
    size_t attribCount = 0; // Also acts as an index.

    // Reg -> attribute table.
    u32 regTable[GLASS_NUM_ATTRIB_REGS];

    format[0] = permutation[0] = permutation[1] = 0;
    format[1] = 0xFFF0000; // Fixed by default (ie. if disabled).

    // Step 1: setup attribute params (type, count, isFixed, etc.).
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];
        if (!(attrib->flags & GLASS_ATTRIB_FLAG_ENABLED))
            continue;

        if (!(attrib->flags & GLASS_ATTRIB_FLAG_FIXED)) {
            // Set buffer params.
            const GPUAttribType attribType = unwrapAttribType(attrib->type);

            if (attribCount < 8) {
                format[0] |= ATTRIBFMT(attribCount, attrib->count, attribType);
            } else {
                format[1] |= ATTRIBFMT(attribCount - 8, attrib->count, attribType);
            }

            // Clear fixed flag.
            format[1] &= ~(1u << (16 + attribCount));
        }

        // Map attribute to input register.
        if (attribCount < 8) {
            permutation[0] |= (regId << (4 * attribCount));
        } else {
            permutation[1] |= (regId << (4  * attribCount));
        }
        
        regTable[regId] = attribCount;
        ++attribCount;
    }

    format[1] |= ((attribCount - 1) << 28);

    // Set the type, num of components, is fixed, and total count of attributes.
    addIncrementalWrites(list, GPUREG_ATTRIBBUFFERS_FORMAT_LOW, format, 2);
    addMaskedWrite(list, GPUREG_VSH_INPUTBUFFER_CONFIG, 0x0B, 0xA0000000 | (attribCount - 1));
    addWrite(list, GPUREG_VSH_NUM_ATTR, attribCount - 1);

    // Map each vertex attribute to an input register.
    addIncrementalWrites(list, GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, permutation, 2);

    // Set buffers base.
    addWrite(list, GPUREG_ATTRIBBUFFERS_LOC, PHYSICAL_BUFFER_BASE >> 3);

    // Step 2: setup fixed attributes.
    // This must be set after initial configuration.
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];
        if (!(attrib->flags & GLASS_ATTRIB_FLAG_ENABLED))
            continue;

        if (attrib->flags & GLASS_ATTRIB_FLAG_FIXED) {
            u32 packed[3];
            GLASS_math_packFloatVector(attrib->components, packed);
            addWrite(list, GPUREG_FIXEDATTRIB_INDEX, regTable[regId]);
            addIncrementalWrites(list, GPUREG_FIXEDATTRIB_DATA0, packed, 3);
        }
    }

    // Step 3: setup attribute buffers.
    size_t currentAttribBuffer = 0;
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];
        if ((!(attrib->flags & GLASS_ATTRIB_FLAG_ENABLED)) || (attrib->flags & GLASS_ATTRIB_FLAG_FIXED))
            continue;
        
        u32 params[3];
        memset(&params, 0, sizeof(params));

        if (attrib->physAddr) {
            // Calculate permutation.
            u32 permutation[2];
            memset(&permutation, 0, sizeof(permutation));

            const size_t permIndex = insertAttribPad(permutation, 0, attrib->sizeOfPrePad);
            const size_t attribIndex = regTable[regId];

            if (permIndex < 8) {
                permutation[0] |= (attribIndex << (permIndex * 4));
            } else {
                permutation[1] |= (attribIndex << (permIndex * 4));
            }

            const size_t numPerms = insertAttribPad(permutation, permIndex + 1, attrib->sizeOfPostPad);

            params[0] = attrib->physAddr - PHYSICAL_BUFFER_BASE;
            params[1] = permutation[0];
            params[2] = permutation[1] | ((attrib->bufferSize & 0xFF) << 16) | ((numPerms & 0xF) << 28);
        }

        addIncrementalWrites(list, GPUREG_ATTRIBBUFFER0_OFFSET + (currentAttribBuffer * 0x03), params, 3);
        ++currentAttribBuffer;
        KYGX_ASSERT(currentAttribBuffer <= 12); // We can have at most 12 attribute buffers.
    }
}

static inline GPUCombinerSource unwrapCombinerSrc(GLenum src) {
    switch (src) {
        case GL_PRIMARY_COLOR:
            return COMBINERSRC_PRIMARY_COLOR;
        case GL_FRAGMENT_PRIMARY_COLOR_PICA:
            return COMBINERSRC_FRAGMENT_PRIMARY_COLOR;
        case GL_FRAGMENT_SECONDARY_COLOR_PICA:
            return COMBINERSRC_FRAGMENT_SECONDARY_COLOR;
        case GL_TEXTURE0:
            return COMBINERSRC_TEXTURE0;
        case GL_TEXTURE1:
            return COMBINERSRC_TEXTURE1;
        case GL_TEXTURE2:
            return COMBINERSRC_TEXTURE2;
            // TODO: what constant to use?
        //case GL_TEXTURE3:
        //    return COMBINERSRC_TEXTURE3;
        case GL_PREVIOUS_BUFFER_PICA:
            return COMBINERSRC_PREVIOUS_BUFFER;
        case GL_CONSTANT:
            return COMBINERSRC_CONSTANT;
        case GL_PREVIOUS:
            return COMBINERSRC_PREVIOUS;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUCombinerOpRGB unwrapCombinerOpRGB(GLenum op) {
    switch (op) {
        case GL_SRC_COLOR:
            return COMBINEROPRGB_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return COMBINEROPRGB_ONE_MINUS_SRC_COLOR;
        case GL_SRC_ALPHA:
            return COMBINEROPRGB_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return COMBINEROPRGB_ONE_MINUS_SRC_ALPHA;
        case GL_SRC_R_PICA:
            return COMBINEROPRGB_SRC_R;
        case GL_ONE_MINUS_SRC_R_PICA:
            return COMBINEROPRGB_ONE_MINUS_SRC_R;
        case GL_SRC_G_PICA:
            return COMBINEROPRGB_SRC_G;
        case GL_ONE_MINUS_SRC_G_PICA:
            return COMBINEROPRGB_ONE_MINUS_SRC_G;
        case GL_SRC_B_PICA:
            return COMBINEROPRGB_SRC_B;
        case GL_ONE_MINUS_SRC_B_PICA:
            return COMBINEROPRGB_ONE_MINUS_SRC_B;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUCombinerOpAlpha unwrapCombinerOpAlpha(GLenum op) {
    switch (op) {
        case GL_SRC_ALPHA:
            return COMBINEROPALPHA_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return COMBINEROPALPHA_ONE_MINUS_SRC_ALPHA;
        case GL_SRC_R_PICA:
            return COMBINEROPALPHA_SRC_R;
        case GL_ONE_MINUS_SRC_R_PICA:
            return COMBINEROPALPHA_ONE_MINUS_SRC_R;
        case GL_SRC_G_PICA:
            return COMBINEROPALPHA_SRC_G;
        case GL_ONE_MINUS_SRC_G_PICA:
            return COMBINEROPALPHA_ONE_MINUS_SRC_G;
        case GL_SRC_B_PICA:
            return COMBINEROPALPHA_SRC_B;
        case GL_ONE_MINUS_SRC_B_PICA:
            return COMBINEROPALPHA_ONE_MINUS_SRC_B;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUCombinerFunc unwrapCombinerFunc(GLenum func) {
    switch (func) {
        case GL_REPLACE:
            return COMBINERFUNC_REPLACE;
        case GL_MODULATE:
            return COMBINERFUNC_MODULATE;
        case GL_ADD:
            return COMBINERFUNC_ADD;
        case GL_ADD_SIGNED:
            return COMBINERFUNC_ADD_SIGNED;
        case GL_INTERPOLATE:
            return COMBINERFUNC_INTERPOLATE;
        case GL_SUBTRACT:
            return COMBINERFUNC_SUBTRACT;
        case GL_DOT3_RGB:
            return COMBINERFUNC_DOT3_RGB;
        case GL_DOT3_RGBA:
            return COMBINERFUNC_DOT3_RGBA;
        case GL_MULT_ADD_PICA:
            return COMBINERFUNC_MULTIPLY_ADD;
        case GL_ADD_MULT_PICA:
            return COMBINERFUNC_ADD_MULTIPLY;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUCombinerScale unwrapCombinerScale(GLfloat scale) {
    if (scale == 1.0f)
        return COMBINERSCALE_1;

    if (scale == 2.0f)
        return COMBINERSCALE_2;

    if (scale == 4.0f)
        return COMBINERSCALE_4;

    KYGX_UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setCombiners(GLASSGPUCommandList* list, const CombinerInfo* combiners) {
    KYGX_ASSERT(combiners);

    const size_t offsets[GLASS_NUM_COMBINER_STAGES] = {
        GPUREG_TEXENV0_SOURCE, GPUREG_TEXENV1_SOURCE, GPUREG_TEXENV2_SOURCE,
        GPUREG_TEXENV3_SOURCE, GPUREG_TEXENV4_SOURCE, GPUREG_TEXENV5_SOURCE,
    };

    for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; ++i) {
        u32 params[5];
        const CombinerInfo* combiner = &combiners[i];

        params[0] = unwrapCombinerSrc(combiner->rgbSrc[0]);
        params[0] |= (unwrapCombinerSrc(combiner->rgbSrc[1]) << 4);
        params[0] |= (unwrapCombinerSrc(combiner->rgbSrc[2]) << 8);
        params[0] |= (unwrapCombinerSrc(combiner->alphaSrc[0]) << 16);
        params[0] |= (unwrapCombinerSrc(combiner->alphaSrc[1]) << 20);
        params[0] |= (unwrapCombinerSrc(combiner->alphaSrc[2]) << 24);
        params[1] = unwrapCombinerOpRGB(combiner->rgbOp[0]);
        params[1] |= (unwrapCombinerOpRGB(combiner->rgbOp[1]) << 4);
        params[1] |= (unwrapCombinerOpRGB(combiner->rgbOp[2]) << 8);
        params[1] |= (unwrapCombinerOpAlpha(combiner->alphaOp[0]) << 12);
        params[1] |= (unwrapCombinerOpAlpha(combiner->alphaOp[1]) << 16);
        params[1] |= (unwrapCombinerOpAlpha(combiner->alphaOp[2]) << 20);
        params[2] = unwrapCombinerFunc(combiner->rgbFunc);
        params[2] |= (unwrapCombinerFunc(combiner->alphaFunc) << 16);
        params[3] = combiner->color;
        params[4] = unwrapCombinerScale(combiner->rgbScale);
        params[4] |= (unwrapCombinerScale(combiner->alphaScale) << 16);

        addIncrementalWrites(list, offsets[i], params, 5);
    }
}

static inline GPUFragOpMode unwrapFragOpMode(GLenum mode) {
    switch (mode) {
        case GL_FRAGOP_MODE_DEFAULT_PICA:
            return FRAGOPMODE_DEFAULT;
        case GL_FRAGOP_MODE_SHADOW_PICA:
            return FRAGOPMODE_SHADOW;
        case GL_FRAGOP_MODE_GAS_PICA:
            return FRAGOPMODE_GAS;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setFragOp(GLASSGPUCommandList* list, GLenum mode, bool blendMode) {
    addMaskedWrite(list, GPUREG_COLOR_OPERATION, 0x07, 0xE40000 | (blendMode ? 0x100 : 0x0) | unwrapFragOpMode(mode));
}

static inline GPUTestFunc unwrapTestFunc(GLenum func) {
    switch (func) {
        case GL_NEVER:
            return TESTFUNC_NEVER;
        case GL_LESS:
            return TESTFUNC_LESS;
        case GL_EQUAL:
            return TESTFUNC_EQUAL;
        case GL_LEQUAL:
            return TESTFUNC_LEQUAL;
        case GL_GREATER:
            return TESTFUNC_GREATER;
        case GL_NOTEQUAL:
            return TESTFUNC_NOTEQUAL;
        case GL_GEQUAL:
            return TESTFUNC_GEQUAL;
        case GL_ALWAYS:
            return TESTFUNC_ALWAYS;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setColorDepthMask(GLASSGPUCommandList* list, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc) {
    u32 value = (writeRed ? 0x0100 : 0x00) | (writeGreen ? 0x0200 : 0x00) | (writeBlue ? 0x0400 : 0x00) | (writeAlpha ? 0x0800 : 0x00);
    value |= (unwrapTestFunc(depthFunc) << 4) | (writeDepth ? 0x1000 : 0x00) | (depthTest ? 1 : 0);
    addMaskedWrite(list, GPUREG_DEPTH_COLOR_MASK, 0x03, value);
}

static inline float getDepthMapOffset(GLenum format, GLfloat units) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
            return units / 65535.0f;
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return units / 16777215.0f;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0.0f;
}

void GLASS_gpu_setDepthMap(GLASSGPUCommandList* list, bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum format) {
    KYGX_ASSERT(nearVal >= 0.0f && nearVal <= 1.0f);
    KYGX_ASSERT(farVal >= 0.0f && farVal <= 1.0f);

    const float offset = getDepthMapOffset(format, units);
    
    addMaskedWrite(list, GPUREG_DEPTHMAP_ENABLE, 0x01, enabled ? 1 : 0);
    addWrite(list, GPUREG_DEPTHMAP_SCALE, GLASS_math_f32tof24(nearVal - farVal));
    addWrite(list, GPUREG_DEPTHMAP_OFFSET, GLASS_math_f32tof24(nearVal + offset));
}

void GLASS_gpu_setEarlyDepthTest(GLASSGPUCommandList* list, bool enabled) {
    addMaskedWrite(list, GPUREG_EARLYDEPTH_TEST1, 0x01, enabled ? 1 : 0);
    addMaskedWrite(list, GPUREG_EARLYDEPTH_TEST2, 0x01, enabled ? 1 : 0);
}

void GLASS_gpu_setEarlyDepthFunc(GLASSGPUCommandList* list, GPUEarlyDepthFunc func) { addMaskedWrite(list, GPUREG_EARLYDEPTH_FUNC, 0x01, func); }

void GLASS_gpu_setEarlyDepthClear(GLASSGPUCommandList* list, GLclampf value) {
    KYGX_ASSERT(value >= 0.0f && value <= 1.0f);
    addMaskedWrite(list, GPUREG_EARLYDEPTH_DATA, 0x07, 0xFFFFFF * value);
}

void GLASS_gpu_clearEarlyDepthBuffer(GLASSGPUCommandList* list) { addWrite(list, GPUREG_EARLYDEPTH_CLEAR, 1); }

void GLASS_gpu_setStencilTest(GLASSGPUCommandList* list, bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask) {
    addWrite(list, GPUREG_STENCIL_TEST, (unwrapTestFunc(func) << 4) | ((u8)writeMask << 8) | ((s8)ref << 16) | ((u8)mask << 24) | (enabled ? 1 : 0));
}

static inline GPUStencilOp unwrapStencilOp(GLenum op) {
    switch (op) {
        case GL_KEEP:
            return STENCILOP_KEEP;
        case GL_ZERO:
            return STENCILOP_ZERO;
        case GL_REPLACE:
            return STENCILOP_REPLACE;
        case GL_INCR:
            return STENCILOP_INCR;
        case GL_INCR_WRAP:
            return STENCILOP_INCR_WRAP;
        case GL_DECR:
            return STENCILOP_DECR;
        case GL_DECR_WRAP:
            return STENCILOP_DECR_WRAP;
        case GL_INVERT:
            return STENCILOP_INVERT;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setStencilOp(GLASSGPUCommandList* list, GLenum sfail, GLenum dpfail, GLenum dppass) { addMaskedWrite(list, GPUREG_STENCIL_OP, 0x03, unwrapStencilOp(sfail) | (unwrapStencilOp(dpfail) << 4) | (unwrapStencilOp(dppass) << 8)); }

void GLASS_gpu_setCullFace(GLASSGPUCommandList* list, bool enabled, GLenum cullFace, GLenum frontFace) {
    // Essentially:
    // - set FRONT-CCW for FRONT-CCW/BACK-CW;
    // - set BACK-CCW in all other cases.
    const GPUCullMode mode = ((cullFace == GL_FRONT) != (frontFace == GL_CCW)) ? CULLMODE_BACK_CCW : CULLMODE_FRONT_CCW;
    addMaskedWrite(list, GPUREG_FACECULLING_CONFIG, 0x01, enabled ? mode : CULLMODE_NONE);
}

void GLASS_gpu_setAlphaTest(GLASSGPUCommandList* list, bool enabled, GLenum func, GLclampf ref) {
    KYGX_ASSERT(ref >= 0.0f && ref <= 1.0f);
    addMaskedWrite(list, GPUREG_FRAGOP_ALPHA_TEST, 0x03, (unwrapTestFunc(func) << 4) | ((u32)(ref * 0xFF) << 8) | (enabled ? 1 : 0));
}

static inline GPUBlendEq unwrapBlendEq(GLenum eq) {
    switch (eq) {
        case GL_FUNC_ADD:
            return BLENDEQ_ADD;
        case GL_MIN:
            return BLENDEQ_MIN;
        case GL_MAX:
            return BLENDEQ_MAX;
        case GL_FUNC_SUBTRACT:
            return BLENDEQ_SUBTRACT;
        case GL_FUNC_REVERSE_SUBTRACT:
            return BLENDEQ_REVERSE_SUBTRACT;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUBlendFactor unwrapBlendFactor(GLenum func) {
    switch (func) {
        case GL_ZERO:
            return BLENDFACTOR_ZERO;
        case GL_ONE:
            return BLENDFACTOR_ONE;
        case GL_SRC_COLOR:
            return BLENDFACTOR_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return BLENDFACTOR_ONE_MINUS_SRC_COLOR;
        case GL_DST_COLOR:
            return BLENDFACTOR_DST_COLOR;
        case GL_ONE_MINUS_DST_COLOR:
            return BLENDFACTOR_ONE_MINUS_DST_COLOR;
        case GL_SRC_ALPHA:
            return BLENDFACTOR_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        case GL_DST_ALPHA:
            return BLENDFACTOR_DST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA:
            return BLENDFACTOR_ONE_MINUS_DST_ALPHA;
        case GL_CONSTANT_COLOR:
            return BLENDFACTOR_CONSTANT_COLOR;
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
        case GL_CONSTANT_ALPHA:
            return BLENDFACTOR_CONSTANT_ALPHA;
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return BLENDFACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case GL_SRC_ALPHA_SATURATE:
            return BLENDFACTOR_SRC_ALPHA_SATURATE;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setBlendFunc(GLASSGPUCommandList* list, GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha) {
    const GPUBlendEq nativeRGBEq = unwrapBlendEq(rgbEq);
    const GPUBlendEq nativeAlphaEq = unwrapBlendEq(alphaEq);
    const GPUBlendFactor nativeSrcColor = unwrapBlendFactor(srcColor);
    const GPUBlendFactor nativeDstColor = unwrapBlendFactor(dstColor);
    const GPUBlendFactor nativeSrcAlpha = unwrapBlendFactor(srcAlpha);
    const GPUBlendFactor nativeDstAlpha = unwrapBlendFactor(dstAlpha);
    addWrite(list, GPUREG_BLEND_FUNC, (nativeDstAlpha << 28) | (nativeSrcAlpha << 24) | (nativeDstColor << 20) | (nativeSrcColor << 16) | (nativeAlphaEq << 8) | nativeRGBEq);
}

void GLASS_gpu_setBlendColor(GLASSGPUCommandList* list, u32 color) { addWrite(list, GPUREG_BLEND_COLOR, color); }

static inline GPULogicOp unwrapLogicOp(GLenum op) {
    switch (op) {
        case GL_CLEAR:
            return LOGICOP_CLEAR;
        case GL_AND:
            return LOGICOP_AND;
        case GL_AND_REVERSE:
            return LOGICOP_AND_REVERSE;
        case GL_COPY:
            return LOGICOP_COPY;
        case GL_AND_INVERTED:
            return LOGICOP_AND_INVERTED;
        case GL_NOOP:
            return LOGICOP_NOOP;
        case GL_XOR:
            return LOGICOP_XOR;
        case GL_OR:
            return LOGICOP_OR;
        case GL_NOR:
            return LOGICOP_NOR;
        case GL_EQUIV:
            return LOGICOP_EQUIV;
        case GL_INVERT:
            return LOGICOP_INVERT;
        case GL_OR_REVERSE:
            return LOGICOP_OR_REVERSE;
        case GL_COPY_INVERTED:
            return LOGICOP_COPY_INVERTED;
        case GL_OR_INVERTED:
            return LOGICOP_OR_INVERTED;
        case GL_NAND:
            return LOGICOP_NAND;
        case GL_SET:
            return LOGICOP_SET;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setLogicOp(GLASSGPUCommandList* list, GLenum op) { addMaskedWrite(list, GPUREG_LOGIC_OP, 0x01, unwrapLogicOp(op)); }

static inline GPUPrimitive unwrapDrawPrimitive(GLenum mode) {
    switch (mode) {
        case GL_TRIANGLES:
            return PRIMITIVE_TRIANGLES;
        case GL_TRIANGLE_STRIP:
            return PRIMITIVE_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:
            return PRIMITIVE_TRIANGLE_FAN;
        case GL_GEOMETRY_PRIMITIVE_PICA:
            return PRIMITIVE_GEOMETRY;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_drawArrays(GLASSGPUCommandList* list, GLenum mode, GLint first, GLsizei count) {
    addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 2, unwrapDrawPrimitive(mode) << 8);
    addWrite(list, GPUREG_RESTART_PRIMITIVE, 1);
    addWrite(list, GPUREG_INDEXBUFFER_CONFIG, 0x80000000);
    addWrite(list, GPUREG_NUMVERTICES, count);
    addWrite(list, GPUREG_VERTEX_OFFSET, first);
    addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 1, 1);
    addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 0);
    addWrite(list, GPUREG_DRAWARRAYS, 1);
    addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 1);
    addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 1, 0);
    addWrite(list, GPUREG_VTX_FUNC, 1);
}

static inline u32 unwrapDrawType(GLenum type) {
    switch (type) {
        case GL_UNSIGNED_BYTE:
            return 0;
        case GL_UNSIGNED_SHORT:
            return 1;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_drawElements(GLASSGPUCommandList* list, GLenum mode, GLsizei count, GLenum type, u32 physIndices) {
    const GPUPrimitive primitive = unwrapDrawPrimitive(mode);

    addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 2, (primitive != PRIMITIVE_TRIANGLES ? primitive : PRIMITIVE_GEOMETRY) << 8);

    addWrite(list, GPUREG_RESTART_PRIMITIVE, 1);
    addWrite(list, GPUREG_INDEXBUFFER_CONFIG, (physIndices - PHYSICAL_BUFFER_BASE) | (unwrapDrawType(type) << 31));

    addWrite(list, GPUREG_NUMVERTICES, count);
    addWrite(list, GPUREG_VERTEX_OFFSET, 0);

    if (primitive == PRIMITIVE_TRIANGLES) {
        addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 2, 0x100);
        addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 2, 0x100);
    }

    addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 0);
    addWrite(list, GPUREG_DRAWELEMENTS, 1);
    addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 1);

    if (primitive == PRIMITIVE_TRIANGLES) {
        addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 2, 0);
        addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 2, 0);
    }

    addWrite(list, GPUREG_VTX_FUNC, 1);
    addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
    addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
}

static inline GPUTexFilter unwrapTexFilter(GLenum filter) {
    switch (filter) {
        case GL_NEAREST:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_NEAREST_MIPMAP_LINEAR:
            return TEXFILTER_NEAREST;
        case GL_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_LINEAR:
            return TEXFILTER_LINEAR;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUTexFilter unwrapMipFilter(GLenum minFilter) {
    switch (minFilter) {
        case GL_NEAREST:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
            return TEXFILTER_NEAREST;
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_LINEAR:
            return TEXFILTER_LINEAR;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

static inline GPUTexWrap unwrapTexWrap(GLenum wrap) {
    switch (wrap) {
        case GL_CLAMP_TO_EDGE:
            return TEXWRAP_CLAMP_TO_EDGE;
        case GL_CLAMP_TO_BORDER:
            return TEXWRAP_CLAMP_TO_BORDER;
        case GL_MIRRORED_REPEAT:
            return TEXWRAP_MIRRORED_REPEAT;
        case GL_REPEAT:
            return TEXWRAP_REPEAT;
        default:
            KYGX_UNREACHABLE("Invalid parameter!");
    }

    return 0;
}

void GLASS_gpu_setTextureUnits(GLASSGPUCommandList* list, const GLuint* units) {
    KYGX_ASSERT(units);

    const u32 setupCmds[3] = { 
        GPUREG_TEXUNIT0_BORDER_COLOR,
        GPUREG_TEXUNIT1_BORDER_COLOR,
        GPUREG_TEXUNIT2_BORDER_COLOR
    };

    const u32 typeCmds[3] = {
        GPUREG_TEXUNIT0_TYPE,
        GPUREG_TEXUNIT1_TYPE,
        GPUREG_TEXUNIT2_TYPE,
    };

    u32 config = (1u << 12);

    for (size_t i = 0; i < GLASS_NUM_TEX_UNITS; ++i) {
        const TextureInfo* tex = (TextureInfo*)units[i];
        if (!tex)
            continue;

        // Enable unit.
        config |= (1u << i);

        // Setup unit info.
        const bool hasCubeMap = (i == 0) && (tex->target == GL_TEXTURE_CUBE_MAP);
        u32 params[10] = {};

        params[0] = tex->borderColor;
        params[1] = ((u32)(tex->width & 0x3FF) << 16) | (tex->height & 0x3FF);

        const GPUTexFilter minFilter = unwrapTexFilter(tex->minFilter);
        const GPUTexFilter magFilter = unwrapTexFilter(tex->magFilter);
        const GPUTexFilter mipFilter = unwrapMipFilter(tex->minFilter);
        const GPUTexWrap wrapS = unwrapTexWrap(tex->wrapS);
        const GPUTexWrap wrapT = unwrapTexWrap(tex->wrapT);

        params[2] = (minFilter << 2) | (magFilter << 1) | (mipFilter << 24) | (wrapS << 12) | (wrapT << 8);
        
        if (tex->format == TEXFORMAT_ETC1)
            params[2] |= (1u << 5);

        if (i == 0) {
            params[2] |= ((tex->target == GL_TEXTURE_CUBE_MAP ? TEXMODE_CUBEMAP : TEXMODE_2D) << 28);
            // TODO: Shadow
        }

        params[3] = GLASS_math_f32tofixed13(tex->lodBias) | (((u32)tex->maxLod & 0x0F) << 16) | (((u32)tex->minLod & 0x0F) << 24);

        const u32 mainTexAddr = kygxGetPhysicalAddress(tex->faces[0]);
        KYGX_ASSERT(mainTexAddr);
        params[4] = mainTexAddr >> 3;

        if (hasCubeMap) {
            const u32 mask = (mainTexAddr >> 3) & ~(0x3FFFFFu);

            for (size_t j = 1; j < GLASS_NUM_TEX_FACES; ++j) {
                const u32 dataAddr = kygxGetPhysicalAddress(tex->faces[j]);
                KYGX_ASSERT(dataAddr);
                KYGX_ASSERT(((dataAddr >> 3) & ~(0x3FFFFFu)) == mask);
                params[4 + j] = (dataAddr >> 3) & 0x3FFFFFu;
            }
        }

        addIncrementalWrites(list, setupCmds[i], params, hasCubeMap ? 10 : 5);
        addWrite(list, typeCmds[i], tex->format);
    }

    addWrite(list, GPUREG_TEXUNIT_CONFIG, config);
    addMaskedWrite(list, GPUREG_TEXUNIT_CONFIG, 0x4, (1u << 16)); // Clear cache.
}