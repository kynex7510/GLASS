#include "Platform/GPU.h"
#include "Base/Utility.h"
#include "Base/Pixels.h"

#include <string.h> // memcpy, memset

#define PHYSICAL_LINEAR_BASE 0x18000000
#define DEFAULT_CMDBUF_CAPACITY 0x1000

#define PAD_4 12
#define PAD_8 13
#define PAD_12 14
#define PAD_16 15

static size_t GLASS_addCmdImplStep(u32* cmdBuffer, u32 header, const u32* params, size_t numParams) {
    ASSERT(cmdBuffer);
    ASSERT(params);
    ASSERT(numParams);
    ASSERT(GLASS_utility_isAligned((size_t)cmdBuffer, 0x8));

    // Write header + first parameter.
    cmdBuffer[0] = params[0];
    cmdBuffer[1] = header;

    // Write other parameters.
    memcpy(&cmdBuffer[2], &params[1], (numParams - 1) * sizeof(u32));

    // Make sure commands are kept aligned.
    if (numParams & 1) {
        ++numParams;
        cmdBuffer[numParams] = 0;
    }

    return (numParams + 1) * sizeof(u32);
}

static void GLASS_addCmdImpl(glassGpuCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams, bool consecutive) {
    ASSERT(list);
    ASSERT(params);
    ASSERT(numParams > 0);
    ASSERT(list->offset + (numParams * sizeof(u32)) < list->capacity);

    for (size_t i = 0; i < numParams; i += 256) {
        u32* cmdBuffer = (u8*)(list->mainBuffer) + list->offset;

        // Calculate current number of parameters and header.
        const size_t curNumParams = MIN(numParams, 255);
        const u32 header = (id & 0xFFFF) | ((mask & 0xF) << 16) | ((curNumParams & 0xFF) << 20) | (consecutive ? (1 << 31) : 0);

        // Write params data.
        list->offset += GLASS_addCmdImplStep(cmdBuffer, header, &params[i], curNumParams);

        // Update id for consecutive writes.
        if (consecutive)
            id += curNumParams;
    }
}

static void GLASS_addMaskedWrites(glassGpuCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams) {
    GLASS_addCmdImpl(list, id, mask, params, numParams, false);
}

static void GLASS_addWrites(glassGpuCommandList* list, u32 id, const u32* params, size_t numParams) { GLASS_addMaskedWrites(list, id, 0xF, params, numParams); };
static void GLASS_addMaskedWrite(glassGpuCommandList* list, u32 id, u32 mask, u32 v) { GLASS_addMaskedWrites(list, id, mask, &v, 1); }
static void GLASS_addWrite(glassGpuCommandList* list, u32 id, u32 v) { GLASS_addMaskedWrite(list, id, 0xF, v); }

static void GLASS_addMaskedIncrementalWrites(glassGpuCommandList* list, u32 id, u32 mask, const u32* params, size_t numParams) {
    GLASS_addCmdImpl(list, id, mask, params, numParams, true);
}

static void GLASS_addIncrementalWrites(glassGpuCommandList* list, u32 id, const u32* params, size_t numParams) {
    GLASS_addMaskedIncrementalWrites(list, id, 0xF, params, numParams);
}

void GLASS_gpu_allocList(glassGpuCommandList* list) {
    ASSERT(list);

    if (!list->capacity) {
        ASSERT(!list->mainBuffer);
        ASSERT(!list->secondBuffer);
        list->capacity = DEFAULT_CMDBUF_CAPACITY;
    } else {
        ASSERT(GLASS_utility_isAligned(list->capacity, 0x10));
    }

    if (!list->mainBuffer) {
        list->mainBuffer = glassLinearAlloc(list->capacity);
        ASSERT(list->mainBuffer);
    } else {
        ASSERT(glassIsLinear(list->mainBuffer));
    }

    if (!list->secondBuffer) {
        list->secondBuffer = glassLinearAlloc(list->capacity);
        ASSERT(list->secondBuffer);
    } else {
        ASSERT(glassIsLinear(list->secondBuffer));
    }
}

void GLASS_gpu_freeList(glassGpuCommandList* list) {
    ASSERT(list);

    glassLinearFree(list->secondBuffer);
    glassLinearFree(list->mainBuffer);
    list->secondBuffer = NULL;
    list->mainBuffer = NULL;
    list->capacity = 0;
    list->offset = 0;
}

bool GLASS_gpu_swapCommandBuffers(glassGpuCommandList* list, void** outBuffer, size_t* outSize) {
    ASSERT(list);
    ASSERT(outBuffer);
    ASSERT(outSize);

    if (list->offset > 0) {
        // Finalize list.
        GLASS_addWrite(list, GPUREG_FINALIZE, 0x12345678);
        if (!GLASS_utility_isAligned(list->offset, 0x10))
            GLASS_addWrite(list, GPUREG_FINALIZE, 0x12345678);

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

static size_t GLASS_unwrapRBPixelSize(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return 2;
        case GL_RGB8_OES:
            return 1;
        case GL_RGB5_A1:
        case GL_RGB565:
        case GL_RGBA4:
            return 0;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_COLORBUF GLASS_unwrapRBFormat(GLenum format) {
    switch (format) {
        case GL_RGBA8_OES:
            return GPU_RB_RGBA8;
        case GL_RGB8_OES:
            return GPU_RB_RGB8;
        case GL_RGB5_A1:
            return GPU_RB_RGBA5551;
        case GL_RGB565:
            return GPU_RB_RGB565;
        case GL_RGBA4:
            return GPU_RB_RGBA4;
        case GL_DEPTH_COMPONENT16:
            return GPU_RB_DEPTH16;
        case GL_DEPTH_COMPONENT24_OES:
            return GPU_RB_DEPTH24;
        case GL_DEPTH24_STENCIL8_OES:
            return GPU_RB_DEPTH24_STENCIL8;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_bindFramebuffer(glassGpuCommandList* list, const FramebufferInfo* info, bool block32) {
    u8* colorBuffer = NULL;
    u8* depthBuffer = NULL;
    u32 width = 0;
    u32 height = 0;
    GLenum colorFormat = 0;
    GLenum depthFormat = 0;
    u32 params[4];

    if (info) {
        const RenderbufferInfo* cb = (RenderbufferInfo*)info->colorBuffer;
        const RenderbufferInfo* db = (RenderbufferInfo*)info->depthBuffer;

        if (cb) {
            colorBuffer = cb->address;
            width = cb->width;
            height = cb->height;
            colorFormat = cb->format;
        }

        if (db) {
            depthBuffer = db->address;
            width = db->width;
            height = db->height;
            depthFormat = db->format;
        }
    }

    GLASS_gpu_invalidateFramebuffer(list);

    // Set depth buffer, color buffer and dimensions.
    params[0] = GLASS_utility_convertVirtToPhys(depthBuffer) >> 3;
    params[1] = GLASS_utility_convertVirtToPhys(colorBuffer) >> 3;
    params[2] = 0x01000000 | (((width - 1) & 0xFFF) << 12) | (height & 0xFFF);
    GLASS_addIncrementalWrites(list, GPUREG_DEPTHBUFFER_LOC, params, 3);
    GLASS_addWrite(list, GPUREG_RENDERBUF_DIM, params[2]);

    // Set buffer parameters.
    if (colorBuffer) {
        GLASS_addWrite(list, GPUREG_COLORBUFFER_FORMAT, (GLASS_unwrapRBFormat(colorFormat) << 16) | GLASS_unwrapRBPixelSize(colorFormat));
        params[0] = params[1] = 0x0F;
    } else {
        params[0] = params[1] = 0;
    }

    if (depthBuffer) {
        GLASS_addWrite(list, GPUREG_DEPTHBUFFER_FORMAT, GLASS_unwrapRBFormat(depthFormat));
        params[2] = params[3] = 0x03;
    } else {
        params[2] = params[3] = 0;
    }

    GLASS_addWrite(list, GPUREG_FRAMEBUFFER_BLOCK32, block32 ? 1 : 0);
    GLASS_addIncrementalWrites(list, GPUREG_COLORBUFFER_READ, params, 4);
}

void GLASS_gpu_flushFramebuffer(glassGpuCommandList* list) { GLASS_addWrite(list, GPUREG_FRAMEBUFFER_FLUSH, 1); }
void GLASS_gpu_invalidateFramebuffer(glassGpuCommandList* list) { GLASS_addWrite(list, GPUREG_FRAMEBUFFER_INVALIDATE, 1); }

void GLASS_gpu_setViewport(glassGpuCommandList* list, GLint x, GLint y, GLsizei width, GLsizei height) {
    u32 data[4];

    data[0] = GLASS_utility_f32tof24(height / 2.0f);
    data[1] = (GLASS_utility_f32tof31(2.0f / height) << 1);
    data[2] = GLASS_utility_f32tof24(width / 2.0f);
    data[3] = (GLASS_utility_f32tof31(2.0f / width) << 1);

    GLASS_addIncrementalWrites(list, GPUREG_VIEWPORT_WIDTH, data, 4);
    GLASS_addWrite(list, GPUREG_VIEWPORT_XY, (x << 16) | (y & 0xFFFF));
}

void GLASS_gpu_setScissorTest(glassGpuCommandList* list, GPU_SCISSORMODE mode, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLASS_addMaskedWrite(list, GPUREG_SCISSORTEST_MODE, 0x01, mode);
    GLASS_addWrite(list, GPUREG_SCISSORTEST_POS, (y << 16) | (x & 0xFFFF));
    GLASS_addWrite(list, GPUREG_SCISSORTEST_DIM, ((width - x - 1) << 16) | ((height - y - 1) & 0xFFFF));
}

static void GLASS_uploadShaderBinary(glassGpuCommandList* list, const ShaderInfo* shader) {
    if (shader->sharedData) {
        // Set write offset for code upload.
        GLASS_addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_CONFIG : GPUREG_VSH_CODETRANSFER_CONFIG, 0);

        // Write code.
        GLASS_addWrites(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_DATA : GPUREG_VSH_CODETRANSFER_DATA,
                        shader->sharedData->binaryCode,
                        shader->sharedData->numOfCodeWords < 512 ? shader->sharedData->numOfCodeWords : 512);

        // Finalize code.
        GLASS_addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_END : GPUREG_VSH_CODETRANSFER_END, 1);

        // Set write offset for op descs.
        GLASS_addWrite(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_CONFIG : GPUREG_VSH_OPDESCS_CONFIG, 0);

        // Write op descs.
        GLASS_addWrites(list, (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_DATA : GPUREG_VSH_OPDESCS_DATA,
                        shader->sharedData->opDescs,
                        shader->sharedData->numOfOpDescs < 128 ? shader->sharedData->numOfOpDescs : 128);
    }
}

void GLASS_gpu_bindShaders(glassGpuCommandList* list, const ShaderInfo* vertexShader, const ShaderInfo* geometryShader) {
    // Initialize geometry engine.
    GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 0x03, geometryShader ? 2 : 0);
    GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 0x03, 0);
    GLASS_addMaskedWrite(list, GPUREG_VSH_COM_MODE, 0x01, geometryShader ? 1 : 0);

    if (vertexShader) {
        GLASS_uploadShaderBinary(list, vertexShader);
        GLASS_addWrite(list, GPUREG_VSH_ENTRYPOINT, 0x7FFF0000 | (vertexShader->codeEntrypoint & 0xFFFF));
        GLASS_addMaskedWrite(list, GPUREG_VSH_OUTMAP_MASK, 0x03, vertexShader->outMask);

        // Set vertex shader outmap number.
        GLASS_addMaskedWrite(list, GPUREG_VSH_OUTMAP_TOTAL1, 0x01, vertexShader->outTotal - 1);
        GLASS_addMaskedWrite(list, GPUREG_VSH_OUTMAP_TOTAL2, 0x01, vertexShader->outTotal - 1);
    }

    if (geometryShader) {
        GLASS_uploadShaderBinary(list, geometryShader);
        GLASS_addWrite(list, GPUREG_GSH_ENTRYPOINT, 0x7FFF0000 | (geometryShader->codeEntrypoint & 0xFFFF));
        GLASS_addMaskedWrite(list, GPUREG_GSH_OUTMAP_MASK, 0x01, geometryShader->outMask);
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
        GLASS_addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x01, mergedOutTotal - 1);
        GLASS_addMaskedWrite(list, GPUREG_SH_OUTMAP_TOTAL, 0x01, mergedOutTotal);
        GLASS_addIncrementalWrites(list, GPUREG_SH_OUTMAP_O0, mergedOutSems, 7);
        GLASS_addMaskedWrite(list, GPUREG_SH_OUTATTR_MODE, 0x01, useTexcoords ? 1 : 0);
        GLASS_addWrite(list, GPUREG_SH_OUTATTR_CLOCK, mergedOutClock);
    }

    // TODO: configure geostage.
    GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 0x0A, 0);
    GLASS_addWrite(list, GPUREG_GSH_MISC0, 0);
    GLASS_addWrite(list, GPUREG_GSH_MISC1, 0);
    GLASS_addWrite(list, GPUREG_GSH_INPUTBUFFER_CONFIG, 0xA0000000);
}

static void GLASS_uploadBoolUniformMask(glassGpuCommandList* list, const ShaderInfo* shader, u16 mask) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_BOOLUNIFORM : GPUREG_VSH_BOOLUNIFORM;
    GLASS_addWrite(list, reg, 0x7FFF0000 | mask);
}

static void GLASS_uploadConstIntUniforms(glassGpuCommandList* list ,const ShaderInfo* shader) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    for (size_t i = 0; i < 4; ++i) {
        if (!((shader->constIntMask >> i) & 1))
            continue;

        GLASS_addWrite(list, reg + i, shader->constIntData[i]);
    }
}

static void GLASS_uploadConstFloatUniforms(glassGpuCommandList* list, const ShaderInfo* shader) {
    const u32 idReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    for (size_t i = 0; i < shader->numOfConstFloatUniforms; ++i) {
        const ConstFloatInfo* uni = &shader->constFloatUniforms[i];
        GLASS_addWrite(list, idReg, uni->ID);
        GLASS_addIncrementalWrites(list, dataReg, uni->data, 3);
    }
}

void GLASS_gpu_uploadConstUniforms(glassGpuCommandList* list, const ShaderInfo* shader) {
    ASSERT(shader);
    GLASS_uploadBoolUniformMask(list, shader, shader->constBoolMask);
    GLASS_uploadConstIntUniforms(list, shader);
    GLASS_uploadConstFloatUniforms(list, shader);
}

static void GLASS_uploadIntUniform(glassGpuCommandList* list, const ShaderInfo* shader, UniformInfo* info) {
    const u32 reg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    if (info->count == 1) {
        GLASS_addWrite(list, reg + info->ID, info->data.value);
    } else {
        GLASS_addIncrementalWrites(list, reg + info->ID, info->data.values, info->count);
    }
}

static void GLASS_uploadFloatUniform(glassGpuCommandList* list, const ShaderInfo* shader, UniformInfo* info) {
    const u32 idReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    // ID is automatically incremented after each write.
    GLASS_addWrite(list, idReg, info->ID);

    for (size_t i = 0; i < info->count; ++i)
        GLASS_addIncrementalWrites(list, dataReg, &info->data.values[i * 3], 3);
}

void GLASS_gpu_uploadUniforms(glassGpuCommandList* list, ShaderInfo* shader) {
    ASSERT(shader);

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
                GLASS_uploadIntUniform(list, shader, uni);
                break;
            case GLASS_UNI_FLOAT:
                GLASS_uploadFloatUniform(list, shader, uni);
                break;
            default:
                UNREACHABLE("Invalid uniform type!");
        }

        uni->dirty = false;
    }

    if (uploadBool)
        GLASS_uploadBoolUniformMask(list, shader, boolMask);
}

static GPU_FORMATS GLASS_unwrapAttribType(GLenum type) {
    switch (type) {
        case GL_BYTE:
            return GPU_BYTE;
        case GL_UNSIGNED_BYTE:
            return GPU_UNSIGNED_BYTE;
        case GL_SHORT:
            return GPU_SHORT;
        case GL_FLOAT:
            return GPU_FLOAT;
    }

    UNREACHABLE("Invalid parameter!");
}

static size_t GLASS_insertAttribPad(u32* permutation, size_t startIndex, size_t padSize) {
    ASSERT(permutation);

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
        ASSERT(index < 12); // We can have at most 12 components.
    }

    return index;
}

void GLASS_gpu_uploadAttributes(glassGpuCommandList* list, const AttributeInfo* attribs) {
    ASSERT(attribs);

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
            const GPU_FORMATS attribType = GLASS_unwrapAttribType(attrib->type);

            if (attribCount < 8) {
                format[0] |= GPU_ATTRIBFMT(attribCount, attrib->count, attribType);
            } else {
                format[1] |= GPU_ATTRIBFMT(attribCount - 8, attrib->count, attribType);
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
    GLASS_addIncrementalWrites(list, GPUREG_ATTRIBBUFFERS_FORMAT_LOW, format, 2);
    GLASS_addMaskedWrite(list, GPUREG_VSH_INPUTBUFFER_CONFIG, 0x0B, 0xA0000000 | (attribCount - 1));
    GLASS_addWrite(list, GPUREG_VSH_NUM_ATTR, attribCount - 1);

    // Map each vertex attribute to an input register.
    GLASS_addIncrementalWrites(list, GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, permutation, 2);

    // Set buffers base.
    GLASS_addWrite(list, GPUREG_ATTRIBBUFFERS_LOC, PHYSICAL_LINEAR_BASE >> 3);

    // Step 2: setup fixed attributes.
    // This must be set after initial configuration.
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];
        if (!(attrib->flags & GLASS_ATTRIB_FLAG_ENABLED))
            continue;

        if (attrib->flags & GLASS_ATTRIB_FLAG_FIXED) {
            u32 packed[3];
            GLASS_utility_packFloatVector(attrib->components, packed);
            GLASS_addWrite(list, GPUREG_FIXEDATTRIB_INDEX, regTable[regId]);
            GLASS_addIncrementalWrites(list, GPUREG_FIXEDATTRIB_DATA0, packed, 3);
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

            const size_t permIndex = GLASS_insertAttribPad(permutation, 0, attrib->sizeOfPrePad);
            const size_t attribIndex = regTable[regId];

            if (permIndex < 8) {
                permutation[0] |= (attribIndex << (permIndex * 4));
            } else {
                permutation[1] |= (attribIndex << (permIndex * 4));
            }

            const size_t numPerms = GLASS_insertAttribPad(permutation, permIndex + 1, attrib->sizeOfPostPad);

            params[0] = attrib->physAddr - PHYSICAL_LINEAR_BASE;
            params[1] = permutation[0];
            params[2] = permutation[1] | ((attrib->bufferSize & 0xFF) << 16) | ((numPerms & 0xF) << 28);
        }

        GLASS_addIncrementalWrites(list, GPUREG_ATTRIBBUFFER0_OFFSET + (currentAttribBuffer * 0x03), params, 3);
        ++currentAttribBuffer;
        ASSERT(currentAttribBuffer <= 12); // We can have at most 12 attribute buffers.
    }
}

static GPU_TEVSRC GLASS_unwrapCombinerSrc(GLenum src) {
    switch (src) {
        case GL_PRIMARY_COLOR:
            return GPU_PRIMARY_COLOR;
        case GL_FRAGMENT_PRIMARY_COLOR_PICA:
            return GPU_FRAGMENT_PRIMARY_COLOR;
        case GL_FRAGMENT_SECONDARY_COLOR_PICA:
            return GPU_FRAGMENT_SECONDARY_COLOR;
        case GL_TEXTURE0:
            return GPU_TEXTURE0;
        case GL_TEXTURE1:
            return GPU_TEXTURE1;
        case GL_TEXTURE2:
            return GPU_TEXTURE2;
            // TODO: what constant to use?
        //case GL_TEXTURE3:
        //    return GPU_TEXTURE3;
        case GL_PREVIOUS_BUFFER_PICA:
            return GPU_PREVIOUS_BUFFER;
        case GL_CONSTANT:
            return GPU_CONSTANT;
        case GL_PREVIOUS:
            return GPU_PREVIOUS;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEVOP_RGB GLASS_unwrapCombinerOpRGB(GLenum op) {
    switch (op) {
        case GL_SRC_COLOR:
            return GPU_TEVOP_RGB_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR;
        case GL_SRC_ALPHA:
            return GPU_TEVOP_RGB_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA;
        case GL_SRC_R_PICA:
            return GPU_TEVOP_RGB_SRC_R;
        case GL_ONE_MINUS_SRC_R_PICA:
            return GPU_TEVOP_RGB_ONE_MINUS_SRC_R;
        case GL_SRC_G_PICA:
            return GPU_TEVOP_RGB_SRC_G;
        case GL_ONE_MINUS_SRC_G_PICA:
            return GPU_TEVOP_RGB_ONE_MINUS_SRC_G;
        case GL_SRC_B_PICA:
            return GPU_TEVOP_RGB_SRC_B;
        case GL_ONE_MINUS_SRC_B_PICA:
            return GPU_TEVOP_RGB_ONE_MINUS_SRC_B;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEVOP_A GLASS_unwrapCombinerOpAlpha(GLenum op) {
    switch (op) {
        case GL_SRC_ALPHA:
            return GPU_TEVOP_A_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA;
        case GL_SRC_R_PICA:
            return GPU_TEVOP_A_SRC_R;
        case GL_ONE_MINUS_SRC_R_PICA:
            return GPU_TEVOP_A_ONE_MINUS_SRC_R;
        case GL_SRC_G_PICA:
            return GPU_TEVOP_A_SRC_G;
        case GL_ONE_MINUS_SRC_G_PICA:
            return GPU_TEVOP_A_ONE_MINUS_SRC_G;
        case GL_SRC_B_PICA:
            return GPU_TEVOP_A_SRC_B;
        case GL_ONE_MINUS_SRC_B_PICA:
            return GPU_TEVOP_A_ONE_MINUS_SRC_B;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_COMBINEFUNC GLASS_unwrapCombinerFunc(GLenum func) {
    switch (func) {
        case GL_REPLACE:
            return GPU_REPLACE;
        case GL_MODULATE:
            return GPU_MODULATE;
        case GL_ADD:
            return GPU_ADD;
        case GL_ADD_SIGNED:
            return GPU_ADD_SIGNED;
        case GL_INTERPOLATE:
            return GPU_INTERPOLATE;
        case GL_SUBTRACT:
            return GPU_SUBTRACT;
        case GL_DOT3_RGB:
            return GPU_DOT3_RGB;
        case GL_DOT3_RGBA:
            return GPU_DOT3_RGBA;
        case GL_MULT_ADD_PICA:
            return GPU_MULTIPLY_ADD;
        case GL_ADD_MULT_PICA:
            return GPU_ADD_MULTIPLY;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEVSCALE GLASS_unwrapCombinerScale(GLfloat scale) {
    if (scale == 1.0f)
        return GPU_TEVSCALE_1;

    if (scale == 2.0f)
        return GPU_TEVSCALE_2;

    if (scale == 4.0f)
        return GPU_TEVSCALE_4;

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setCombiners(glassGpuCommandList* list, const CombinerInfo* combiners) {
    ASSERT(combiners);

    const size_t offsets[GLASS_NUM_COMBINER_STAGES] = {
        GPUREG_TEXENV0_SOURCE, GPUREG_TEXENV1_SOURCE, GPUREG_TEXENV2_SOURCE,
        GPUREG_TEXENV3_SOURCE, GPUREG_TEXENV4_SOURCE, GPUREG_TEXENV5_SOURCE,
    };

    for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; ++i) {
        u32 params[5];
        const CombinerInfo* combiner = &combiners[i];

        params[0] = GLASS_unwrapCombinerSrc(combiner->rgbSrc[0]);
        params[0] |= (GLASS_unwrapCombinerSrc(combiner->rgbSrc[1]) << 4);
        params[0] |= (GLASS_unwrapCombinerSrc(combiner->rgbSrc[2]) << 8);
        params[0] |= (GLASS_unwrapCombinerSrc(combiner->alphaSrc[0]) << 16);
        params[0] |= (GLASS_unwrapCombinerSrc(combiner->alphaSrc[1]) << 20);
        params[0] |= (GLASS_unwrapCombinerSrc(combiner->alphaSrc[2]) << 24);
        params[1] = GLASS_unwrapCombinerOpRGB(combiner->rgbOp[0]);
        params[1] |= (GLASS_unwrapCombinerOpRGB(combiner->rgbOp[1]) << 4);
        params[1] |= (GLASS_unwrapCombinerOpRGB(combiner->rgbOp[2]) << 8);
        params[1] |= (GLASS_unwrapCombinerOpAlpha(combiner->alphaOp[0]) << 12);
        params[1] |= (GLASS_unwrapCombinerOpAlpha(combiner->alphaOp[1]) << 16);
        params[1] |= (GLASS_unwrapCombinerOpAlpha(combiner->alphaOp[2]) << 20);
        params[2] = GLASS_unwrapCombinerFunc(combiner->rgbFunc);
        params[2] |= (GLASS_unwrapCombinerFunc(combiner->alphaFunc) << 16);
        params[3] = combiner->color;
        params[4] = GLASS_unwrapCombinerScale(combiner->rgbScale);
        params[4] |= (GLASS_unwrapCombinerScale(combiner->alphaScale) << 16);

        GLASS_addIncrementalWrites(list, offsets[i], params, 5);
    }
}

static GPU_FRAGOPMODE GLASS_unwrapFragOpMode(GLenum mode) {
    switch (mode) {
        case GL_FRAGOP_MODE_DEFAULT_PICA:
            return GPU_FRAGOPMODE_GL;
        case GL_FRAGOP_MODE_SHADOW_PICA:
            return GPU_FRAGOPMODE_SHADOW;
        case GL_FRAGOP_MODE_GAS_PICA:
            return GPU_FRAGOPMODE_GAS_ACC;
            break;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setFragOp(glassGpuCommandList* list, GLenum mode, bool blendMode) {
    GLASS_addMaskedWrite(list, GPUREG_COLOR_OPERATION, 0x07, 0xE40000 | (blendMode ? 0x100 : 0x0) | GLASS_unwrapFragOpMode(mode));
}

static GPU_TESTFUNC GLASS_unwrapTestFunc(GLenum func) {
    switch (func) {
        case GL_NEVER:
            return GPU_NEVER;
        case GL_LESS:
            return GPU_LESS;
        case GL_EQUAL:
            return GPU_EQUAL;
        case GL_LEQUAL:
            return GPU_LEQUAL;
        case GL_GREATER:
            return GPU_GREATER;
        case GL_NOTEQUAL:
            return GPU_NOTEQUAL;
        case GL_GEQUAL:
            return GPU_GEQUAL;
        case GL_ALWAYS:
            return GPU_ALWAYS;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setColorDepthMask(glassGpuCommandList* list, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc) {
    u32 value = (writeRed ? 0x0100 : 0x00) | (writeGreen ? 0x0200 : 0x00) | (writeBlue ? 0x0400 : 0x00) | (writeAlpha ? 0x0800 : 0x00);
    value |= (GLASS_unwrapTestFunc(depthFunc) << 4) | (writeDepth ? 0x1000 : 0x00) | (depthTest ? 1 : 0);
    GLASS_addMaskedWrite(list, GPUREG_DEPTH_COLOR_MASK, 0x03, value);
}

static float GLASS_getDepthMapOffset(GLenum format, GLfloat units) {
    switch (format) {
        case GL_DEPTH_COMPONENT16:
            return units / 65535.0f;
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return units / 16777215.0f;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setDepthMap(glassGpuCommandList* list, bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum format) {
    ASSERT(nearVal >= 0.0f && nearVal <= 1.0f);
    ASSERT(farVal >= 0.0f && farVal <= 1.0f);

    const float offset = GLASS_getDepthMapOffset(format, units);
    
    GLASS_addMaskedWrite(list, GPUREG_DEPTHMAP_ENABLE, 0x01, enabled ? 1 : 0);
    GLASS_addWrite(list, GPUREG_DEPTHMAP_SCALE, GLASS_utility_f32tof24(nearVal - farVal));
    GLASS_addWrite(list, GPUREG_DEPTHMAP_OFFSET, GLASS_utility_f32tof24(nearVal + offset));
}

void GLASS_gpu_setEarlyDepthTest(glassGpuCommandList* list, bool enabled) {
    GLASS_addMaskedWrite(list, GPUREG_EARLYDEPTH_TEST1, 0x01, enabled ? 1 : 0);
    GLASS_addMaskedWrite(list, GPUREG_EARLYDEPTH_TEST2, 0x01, enabled ? 1 : 0);
}

void GLASS_gpu_setEarlyDepthFunc(glassGpuCommandList* list, GPU_EARLYDEPTHFUNC func) { GLASS_addMaskedWrite(list, GPUREG_EARLYDEPTH_FUNC, 0x01, func); }

void GLASS_gpu_setEarlyDepthClear(glassGpuCommandList* list, GLclampf value) {
    ASSERT(value >= 0.0f && value <= 1.0f);
    GLASS_addMaskedWrite(list, GPUREG_EARLYDEPTH_DATA, 0x07, 0xFFFFFF * value);
}

void GLASS_gpu_clearEarlyDepthBuffer(glassGpuCommandList* list) { GLASS_addWrite(list, GPUREG_EARLYDEPTH_CLEAR, 1); }

void GLASS_gpu_setStencilTest(glassGpuCommandList* list, bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask) {
    GLASS_addWrite(list, GPUREG_STENCIL_TEST, (GLASS_unwrapTestFunc(func) << 4) | ((u8)writeMask << 8) | ((s8)ref << 16) | ((u8)mask << 24) | (enabled ? 1 : 0));
}

static GPU_STENCILOP GLASS_unwrapStencilOp(GLenum op) {
    switch (op) {
        case GL_KEEP:
            return GPU_STENCIL_KEEP;
        case GL_ZERO:
            return GPU_STENCIL_ZERO;
        case GL_REPLACE:
            return GPU_STENCIL_REPLACE;
        case GL_INCR:
            return GPU_STENCIL_INCR;
        case GL_INCR_WRAP:
            return GPU_STENCIL_INCR_WRAP;
        case GL_DECR:
            return GPU_STENCIL_DECR;
        case GL_DECR_WRAP:
            return GPU_STENCIL_DECR_WRAP;
        case GL_INVERT:
            return GPU_STENCIL_INVERT;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setStencilOp(glassGpuCommandList* list, GLenum sfail, GLenum dpfail, GLenum dppass) { GLASS_addMaskedWrite(list, GPUREG_STENCIL_OP, 0x03, GLASS_unwrapStencilOp(sfail) | (GLASS_unwrapStencilOp(dpfail) << 4) | (GLASS_unwrapStencilOp(dppass) << 8)); }

void GLASS_gpu_setCullFace(glassGpuCommandList* list, bool enabled, GLenum cullFace, GLenum frontFace) {
    // Essentially:
    // - set FRONT-CCW for FRONT-CCW/BACK-CW;
    // - set BACK-CCW in all other cases.
    const GPU_CULLMODE mode = ((cullFace == GL_FRONT) != (frontFace == GL_CCW)) ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
    GLASS_addMaskedWrite(list, GPUREG_FACECULLING_CONFIG, 0x01, enabled ? mode : GPU_CULL_NONE);
}

void GLASS_gpu_setAlphaTest(glassGpuCommandList* list, bool enabled, GLenum func, GLclampf ref) {
    ASSERT(ref >= 0.0f && ref <= 1.0f);
    GLASS_addMaskedWrite(list, GPUREG_FRAGOP_ALPHA_TEST, 0x03, (GLASS_unwrapTestFunc(func) << 4) | ((u32)(ref * 0xFF) << 8) | (enabled ? 1 : 0));
}

static GPU_BLENDEQUATION GLASS_unwrapBlendEq(GLenum eq) {
    switch (eq) {
        case GL_FUNC_ADD:
            return GPU_BLEND_ADD;
        case GL_MIN:
            return GPU_BLEND_MIN;
        case GL_MAX:
            return GPU_BLEND_MAX;
        case GL_FUNC_SUBTRACT:
            return GPU_BLEND_SUBTRACT;
        case GL_FUNC_REVERSE_SUBTRACT:
            return GPU_BLEND_REVERSE_SUBTRACT;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_BLENDFACTOR GLASS_unwrapBlendFactor(GLenum func) {
    switch (func) {
        case GL_ZERO:
            return GPU_ZERO;
        case GL_ONE:
            return GPU_ONE;
        case GL_SRC_COLOR:
            return GPU_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return GPU_ONE_MINUS_SRC_COLOR;
        case GL_DST_COLOR:
            return GPU_DST_COLOR;
        case GL_ONE_MINUS_DST_COLOR:
            return GPU_ONE_MINUS_DST_COLOR;
        case GL_SRC_ALPHA:
            return GPU_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return GPU_ONE_MINUS_SRC_ALPHA;
        case GL_DST_ALPHA:
            return GPU_DST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA:
            return GPU_ONE_MINUS_DST_ALPHA;
        case GL_CONSTANT_COLOR:
            return GPU_CONSTANT_COLOR;
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return GPU_ONE_MINUS_CONSTANT_COLOR;
        case GL_CONSTANT_ALPHA:
            return GPU_CONSTANT_ALPHA;
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return GPU_ONE_MINUS_CONSTANT_ALPHA;
        case GL_SRC_ALPHA_SATURATE:
            return GPU_SRC_ALPHA_SATURATE;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setBlendFunc(glassGpuCommandList* list, GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha) {
    const GPU_BLENDEQUATION gpuRGBEq = GLASS_unwrapBlendEq(rgbEq);
    const GPU_BLENDEQUATION gpuAlphaEq = GLASS_unwrapBlendEq(alphaEq);
    const GPU_BLENDFACTOR gpuSrcColor = GLASS_unwrapBlendFactor(srcColor);
    const GPU_BLENDFACTOR gpuDstColor = GLASS_unwrapBlendFactor(dstColor);
    const GPU_BLENDFACTOR gpuSrcAlpha = GLASS_unwrapBlendFactor(srcAlpha);
    const GPU_BLENDFACTOR gpuDstAlpha = GLASS_unwrapBlendFactor(dstAlpha);
    GLASS_addWrite(list, GPUREG_BLEND_FUNC, (gpuDstAlpha << 28) | (gpuSrcAlpha << 24) | (gpuDstColor << 20) | (gpuSrcColor << 16) | (gpuAlphaEq << 8) | gpuRGBEq);
}

void GLASS_gpu_setBlendColor(glassGpuCommandList* list, u32 color) { GLASS_addWrite(list, GPUREG_BLEND_COLOR, color); }

static GPU_LOGICOP GLASS_unwrapLogicOp(GLenum op) {
    switch (op) {
        case GL_CLEAR:
            return GPU_LOGICOP_CLEAR;
        case GL_AND:
            return GPU_LOGICOP_AND;
        case GL_AND_REVERSE:
            return GPU_LOGICOP_AND_REVERSE;
        case GL_COPY:
            return GPU_LOGICOP_COPY;
        case GL_AND_INVERTED:
            return GPU_LOGICOP_AND_INVERTED;
        case GL_NOOP:
            return GPU_LOGICOP_NOOP;
        case GL_XOR:
            return GPU_LOGICOP_XOR;
        case GL_OR:
            return GPU_LOGICOP_OR;
        case GL_NOR:
            return GPU_LOGICOP_NOR;
        case GL_EQUIV:
            return GPU_LOGICOP_EQUIV;
        case GL_INVERT:
            return GPU_LOGICOP_INVERT;
        case GL_OR_REVERSE:
            return GPU_LOGICOP_OR_REVERSE;
        case GL_COPY_INVERTED:
            return GPU_LOGICOP_COPY_INVERTED;
        case GL_OR_INVERTED:
            return GPU_LOGICOP_OR_INVERTED;
        case GL_NAND:
            return GPU_LOGICOP_NAND;
        case GL_SET:
            return GPU_LOGICOP_SET;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setLogicOp(glassGpuCommandList* list, GLenum op) { GLASS_addMaskedWrite(list, GPUREG_LOGIC_OP, 0x01, GLASS_unwrapLogicOp(op)); }

static GPU_Primitive_t GLASS_unwrapDrawPrimitive(GLenum mode) {
    switch (mode) {
        case GL_TRIANGLES:
            return GPU_TRIANGLES;
        case GL_TRIANGLE_STRIP:
            return GPU_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:
            return GPU_TRIANGLE_FAN;
        case GL_GEOMETRY_PRIMITIVE_PICA:
            return GPU_GEOMETRY_PRIM;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_drawArrays(glassGpuCommandList* list, GLenum mode, GLint first, GLsizei count) {
    GLASS_addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 2, GLASS_unwrapDrawPrimitive(mode));
    GLASS_addWrite(list, GPUREG_RESTART_PRIMITIVE, 1);
    GLASS_addWrite(list, GPUREG_INDEXBUFFER_CONFIG, 0x80000000);
    GLASS_addWrite(list, GPUREG_NUMVERTICES, count);
    GLASS_addWrite(list, GPUREG_VERTEX_OFFSET, first);
    GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 1, 1);
    GLASS_addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 0);
    GLASS_addWrite(list, GPUREG_DRAWARRAYS, 1);
    GLASS_addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 1);
    GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 1, 0);
    GLASS_addWrite(list, GPUREG_VTX_FUNC, 1);
}

static u32 GLASS_unwrapDrawType(GLenum type) {
    switch (type) {
        case GL_UNSIGNED_BYTE:
            return 0;
        case GL_UNSIGNED_SHORT:
            return 1;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_drawElements(glassGpuCommandList* list, GLenum mode, GLsizei count, GLenum type, u32 physIndices) {
    const GPU_Primitive_t primitive = GLASS_unwrapDrawPrimitive(mode);

    GLASS_addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 2, primitive != GPU_TRIANGLES ? primitive : GPU_GEOMETRY_PRIM);

    GLASS_addWrite(list, GPUREG_RESTART_PRIMITIVE, 1);
    GLASS_addWrite(list, GPUREG_INDEXBUFFER_CONFIG, (physIndices - PHYSICAL_LINEAR_BASE) | (GLASS_unwrapDrawType(type) << 31));

    GLASS_addWrite(list, GPUREG_NUMVERTICES, count);
    GLASS_addWrite(list, GPUREG_VERTEX_OFFSET, 0);

    if (primitive == GPU_TRIANGLES) {
        GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 2, 0x100);
        GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 2, 0x100);
    }

    GLASS_addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 0);
    GLASS_addWrite(list, GPUREG_DRAWELEMENTS, 1);
    GLASS_addMaskedWrite(list, GPUREG_START_DRAW_FUNC0, 1, 1);

    if (primitive == GPU_TRIANGLES) {
        GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG, 2, 0);
        GLASS_addMaskedWrite(list, GPUREG_GEOSTAGE_CONFIG2, 2, 0);
    }

    GLASS_addWrite(list, GPUREG_VTX_FUNC, 1);
    GLASS_addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
    GLASS_addMaskedWrite(list, GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
}

static GPU_TEXTURE_FILTER_PARAM GLASS_unwrapTexFilter(GLenum filter) {
    switch (filter) {
        case GL_NEAREST:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_NEAREST_MIPMAP_LINEAR:
            return GPU_NEAREST;
        case GL_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_LINEAR:
            return GPU_LINEAR;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEXTURE_FILTER_PARAM GLASS_unwrapMipFilter(GLenum minFilter) {
    switch (minFilter) {
        case GL_NEAREST:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
            return GPU_NEAREST;
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_LINEAR:
            return GPU_LINEAR;
    }

    UNREACHABLE("Invalid parameter!");
}

static GPU_TEXTURE_WRAP_PARAM GLASS_unwrapTexWrap(GLenum wrap) {
    switch (wrap) {
        case GL_CLAMP_TO_EDGE:
            return GPU_CLAMP_TO_EDGE;
        case GL_CLAMP_TO_BORDER:
            return GPU_CLAMP_TO_BORDER;
        case GL_MIRRORED_REPEAT:
            return GPU_MIRRORED_REPEAT;
        case GL_REPEAT:
            return GPU_REPEAT;
    }

    UNREACHABLE("Invalid parameter!");
}

void GLASS_gpu_setTextureUnits(glassGpuCommandList* list, const GLuint* units) {
    ASSERT(units);

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

        const GPU_TEXTURE_FILTER_PARAM minFilter = GLASS_unwrapTexFilter(tex->minFilter);
        const GPU_TEXTURE_FILTER_PARAM magFilter = GLASS_unwrapTexFilter(tex->magFilter);
        const GPU_TEXTURE_FILTER_PARAM mipFilter = GLASS_unwrapMipFilter(tex->minFilter);
        const GPU_TEXTURE_WRAP_PARAM wrapS = GLASS_unwrapTexWrap(tex->wrapS);
        const GPU_TEXTURE_WRAP_PARAM wrapT = GLASS_unwrapTexWrap(tex->wrapT);

        params[2] = (GPU_TEXTURE_MIN_FILTER(minFilter) | GPU_TEXTURE_MAG_FILTER(magFilter) | GPU_TEXTURE_MIP_FILTER(mipFilter) | GPU_TEXTURE_WRAP_S(wrapS) | GPU_TEXTURE_WRAP_T(wrapT));
        
        if (tex->pixelFormat.format == GL_ETC1_RGB8_OES)
            params[2] |= GPU_TEXTURE_ETC1_PARAM;

        if (i == 0) {
            params[2] |= GPU_TEXTURE_MODE(tex->target == GL_TEXTURE_CUBE_MAP ? GPU_TEX_CUBE_MAP : GPU_TEX_2D);
            // TODO: Shadow
        }

        params[3] = GLASS_utility_f32tofixed13(tex->lodBias) | (((u32)tex->maxLod & 0x0F) << 16) | (((u32)tex->minLod & 0x0F) << 24);

        const u32 mainTexAddr = GLASS_utility_convertVirtToPhys(tex->faces[0]);
        ASSERT(mainTexAddr);
        params[4] = mainTexAddr >> 3;

        if (hasCubeMap) {
            const u32 mask = (mainTexAddr >> 3) & ~(0x3FFFFFu);

            for (size_t j = 1; j < GLASS_NUM_TEX_FACES; ++j) {
                const u32 dataAddr = GLASS_utility_convertVirtToPhys(tex->faces[j]);
                ASSERT(dataAddr);
                ASSERT(((dataAddr >> 3) & ~(0x3FFFFFu)) == mask);
                params[4 + j] = (dataAddr >> 3) & 0x3FFFFFu;
            }
        }

        GLASS_addIncrementalWrites(list, setupCmds[i], params, hasCubeMap ? 10 : 5);
        const GPU_TEXCOLOR format = GLASS_pixels_tryUnwrapTexFormat(&tex->pixelFormat);
        ASSERT(format != GLASS_INVALID_TEX_FORMAT);
        GLASS_addWrite(list, typeCmds[i], format);
    }

    GLASS_addWrite(list, GPUREG_TEXUNIT_CONFIG, config);
    GLASS_addMaskedWrite(list, GPUREG_TEXUNIT_CONFIG, 0x4, (1u << 16)); // Clear cache.
}