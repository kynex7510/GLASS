#include "GPU.h"
#include "Utility.h"

#include <stdlib.h> // qsort
#include <string.h> // memcpy

#define PHYSICAL_LINEAR_BASE 0x18000000
#define GPU_MAX_ENTRIES 0x4000
#define GX_MAX_ENTRIES 32

typedef struct {
    u32 physAddr;                                    // Physical address to buffer.
    GLsizei stride;                                  // Buffer stride.
    GLsizei numComponents;                           // Number of components.
    u32 components[GLASS_MAX_ENABLED_ATTRIBS];       // Buffer components (attributes), sorted by offset.
    u32 componentOffsets[GLASS_MAX_ENABLED_ATTRIBS]; // Component offsets.
} AttributeBuffer;

void GLASS_gpu_init(CtxCommon* ctx) {
    ASSERT(ctx);

    ctx->cmdBuffer = glassLinearAlloc(GPU_MAX_ENTRIES * sizeof(u32));
    ASSERT(ctx->cmdBuffer);

    ctx->gxQueue.maxEntries = GX_MAX_ENTRIES;
    ctx->gxQueue.entries = glassVirtualAlloc(ctx->gxQueue.maxEntries * sizeof(gxCmdEntry_s));
    ASSERT(ctx->gxQueue.entries);
}

void GLASS_gpu_finalize(CtxCommon* ctx) {
    ASSERT(ctx);
    glassVirtualFree(ctx->gxQueue.entries);
    glassLinearFree(ctx->cmdBuffer);
}

void GLASS_gpu_enableCommands(CtxCommon* ctx) {
    ASSERT(ctx);
    GPUCMD_SetBuffer(ctx->cmdBuffer + ctx->cmdBufferOffset, GPU_MAX_ENTRIES - ctx->cmdBufferOffset, ctx->cmdBufferSize);
}

void GLASS_gpu_disableCommands(CtxCommon* ctx) {
    ASSERT(ctx);
    GPUCMD_GetBuffer(NULL, NULL, &ctx->cmdBufferSize);
    GPUCMD_SetBuffer(NULL, 0, 0);
}

void GLASS_gpu_flushCommands(CtxCommon* ctx) {
    ASSERT(ctx);

    if (ctx->cmdBufferSize) {
        // Split command buffer.
        GLASS_gpu_enableCommands(ctx);
        GPUCMD_Split(NULL, &ctx->cmdBufferSize);
        GPUCMD_SetBuffer(NULL, 0, 0);

        // Process GPU commands.
        u8 flags = GX_CMDLIST_FLUSH;

        if (ctx->initParams.flushAllLinearMem) {
            extern u32 __ctru_linear_heap;
            extern u32 __ctru_linear_heap_size;
            ASSERT(R_SUCCEEDED(GSPGPU_FlushDataCache((void*)__ctru_linear_heap, __ctru_linear_heap_size)));
            flags = 0;
        }

        GX_ProcessCommandList(ctx->cmdBuffer + ctx->cmdBufferOffset, ctx->cmdBufferSize * sizeof(u32), flags);

        // Set new offset.
        ctx->cmdBufferOffset += ctx->cmdBufferSize;
        ctx->cmdBufferSize = 0;
    }
}

void GLASS_gpu_flushQueue(CtxCommon* ctx, bool unbind) {
    ASSERT(ctx);
    gxCmdQueueWait(&ctx->gxQueue, -1);
    gxCmdQueueStop(&ctx->gxQueue);
    gxCmdQueueClear(&ctx->gxQueue);

    if (unbind)
        GX_BindQueue(NULL);
}

void GLASS_gpu_runQueue(CtxCommon* ctx, bool bind) {
    ASSERT(ctx);

    if (bind)
        GX_BindQueue(&ctx->gxQueue);

    gxCmdQueueRun(&ctx->gxQueue);
}

void GLASS_gpu_flushAndRunCommands(CtxCommon* ctx) {
    ASSERT(ctx);

    GLASS_gpu_flushCommands(ctx);
    GLASS_gpu_flushQueue(ctx, false);

    // Reset offset.
    ctx->cmdBufferOffset = 0;
    GLASS_gpu_runQueue(ctx, false);
}

static INLINE u16 GLASS_getGXControl(bool start, bool finished, GLenum format) {
    const u16 fillWidth = GLASS_utility_getPixelSizeForFB(format);
    return (u16)(start) | ((u16)(finished) << 1) | (fillWidth << 8);
}

void GLASS_gpu_clearBuffers(RenderbufferInfo* colorBuffer, u32 clearColor, RenderbufferInfo* depthBuffer, u32 clearDepth) {
    size_t colorBufferSize = 0;
    size_t depthBufferSize = 0;

    if (colorBuffer)
        colorBufferSize = colorBuffer->width * colorBuffer->height * GLASS_utility_getBytesPerPixel(colorBuffer->format);

    if (depthBuffer)
        depthBufferSize = depthBuffer->width * depthBuffer->height * GLASS_utility_getBytesPerPixel(depthBuffer->format);

    if (colorBufferSize && depthBufferSize) {
        const bool colorFirst = (u32)colorBuffer->address < (u32)depthBuffer->address;
        if (colorFirst) {
            GX_MemoryFill((u32*)colorBuffer->address, clearColor, (u32*)(colorBuffer->address + colorBufferSize), GLASS_getGXControl(true, false, colorBuffer->format),
                (u32*)(depthBuffer->address), clearDepth, (u32*)(depthBuffer->address + depthBufferSize), GLASS_getGXControl(true, false, depthBuffer->format));
        } else {
            GX_MemoryFill((u32*)(depthBuffer->address), clearDepth, (u32*)(depthBuffer->address + depthBufferSize), GLASS_getGXControl(true, false, depthBuffer->format),
                (u32*)(colorBuffer->address), clearColor, (u32*)(colorBuffer->address + colorBufferSize), GLASS_getGXControl(true, false, colorBuffer->format));
        }
    } else if (colorBufferSize) {
        GX_MemoryFill((u32*)(colorBuffer->address), clearColor, (u32*)(colorBuffer->address + colorBufferSize), GLASS_getGXControl(true, false, colorBuffer->format), NULL, 0, NULL, 0);
    } else if (depthBufferSize) {
        GX_MemoryFill((u32*)(depthBuffer->address), clearDepth, (u32*)(depthBuffer->address + depthBufferSize), GLASS_getGXControl(true, false, depthBuffer->format), NULL, 0, NULL, 0);
    }
}

void GLASS_gpu_transferBuffer(const RenderbufferInfo* colorBuffer, const RenderbufferInfo* displayBuffer, u32 flags) {
    ASSERT(colorBuffer);
    ASSERT(displayBuffer);
    GX_DisplayTransfer((u32*)(colorBuffer->address), GX_BUFFER_DIM(colorBuffer->height, colorBuffer->width),
        (u32*)(displayBuffer->address), GX_BUFFER_DIM(displayBuffer->height, displayBuffer->width), flags);
}

void GLASS_gpu_bindFramebuffer(const FramebufferInfo* info, bool block32) {
    u8* colorBuffer = NULL;
    u8* depthBuffer = NULL;
    u32 width = 0;
    u32 height = 0;
    GLenum colorFormat = 0;
    GLenum depthFormat = 0;
    u32 params[4] = {};

    if (info) {
        if (info->colorBuffer) {
            colorBuffer = info->colorBuffer->address;
            width = info->colorBuffer->width;
            height = info->colorBuffer->height;
            colorFormat = info->colorBuffer->format;
        }

        if (info->depthBuffer) {
            depthBuffer = info->depthBuffer->address;
            depthFormat = info->depthBuffer->format;

            if (!info->colorBuffer) {
                width = info->colorBuffer->width;
                height = info->colorBuffer->height;
            }
        }
    }

    GLASS_gpu_invalidateFramebuffer();

    // Set depth buffer, color buffer and dimensions.
    params[0] = osConvertVirtToPhys(depthBuffer) >> 3;
    params[1] = osConvertVirtToPhys(colorBuffer) >> 3;
    params[2] = 0x01000000 | (((width - 1) & 0xFFF) << 12) | (height & 0xFFF);
    GPUCMD_AddIncrementalWrites(GPUREG_DEPTHBUFFER_LOC, params, 3);
    GPUCMD_AddWrite(GPUREG_RENDERBUF_DIM, params[2]);

    // Set buffer parameters.
    if (colorBuffer) {
        GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT, (GLASS_utility_getRBFormat(colorFormat) << 16) | GLASS_utility_getPixelSizeForFB(colorFormat));
        params[0] = params[1] = 0x0F;
    } else {
        params[0] = params[1] = 0;
    }

    if (depthBuffer) {
        GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT, GLASS_utility_getRBFormat(depthFormat));
        params[2] = params[3] = 0x03;
    } else {
        params[2] = params[3] = 0;
    }

    if (info)
        GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, block32 ? 1 : 0);

    GPUCMD_AddIncrementalWrites(GPUREG_COLORBUFFER_READ, params, 4);
}

void GLASS_gpu_flushFramebuffer(void) { GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1); }
void GLASS_gpu_invalidateFramebuffer(void) { GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1); }

void GLASS_gpu_setViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    u32 data[4];

    data[0] = f32tof24(height / 2.0f);
    data[1] = (f32tof31(2.0f / height) << 1);
    data[2] = f32tof24(width / 2.0f);
    data[3] = (f32tof31(2.0f / width) << 1);

    GPUCMD_AddIncrementalWrites(GPUREG_VIEWPORT_WIDTH, data, 4);
    GPUCMD_AddWrite(GPUREG_VIEWPORT_XY, (y << 16) | (x & 0xFFFF));
}

void GLASS_gpu_setScissorTest(GPU_SCISSORMODE mode, GLint x, GLint y, GLsizei width, GLsizei height) {
    GPUCMD_AddMaskedWrite(GPUREG_SCISSORTEST_MODE, 0x01, mode);
    GPUCMD_AddWrite(GPUREG_SCISSORTEST_POS, (y << 16) | (x & 0xFFFF));
    GPUCMD_AddWrite(GPUREG_SCISSORTEST_DIM, ((height - y - 1) << 16) | ((width - x - 1) & 0xFFFF));
}

static void GLASS_uploadShaderBinary(const ShaderInfo* shader) {
    if (shader->sharedData) {
        // Set write offset for code upload.
        GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_CONFIG : GPUREG_VSH_CODETRANSFER_CONFIG, 0);

        // Write code.
        GPUCMD_AddWrites((shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_DATA : GPUREG_VSH_CODETRANSFER_DATA,
                        shader->sharedData->binaryCode,
                        shader->sharedData->numOfCodeWords < 512 ? shader->sharedData->numOfCodeWords : 512);

        // Finalize code.
        GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_CODETRANSFER_END : GPUREG_VSH_CODETRANSFER_END, 1);

        // Set write offset for op descs.
        GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_CONFIG : GPUREG_VSH_OPDESCS_CONFIG, 0);

        // Write op descs.
        GPUCMD_AddWrites((shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_OPDESCS_DATA : GPUREG_VSH_OPDESCS_DATA,
                        shader->sharedData->opDescs,
                        shader->sharedData->numOfOpDescs < 128 ? shader->sharedData->numOfOpDescs : 128);
    }
}

void GLASS_gpu_bindShaders(const ShaderInfo* vertexShader, const ShaderInfo* geometryShader) {
    // Initialize geometry engine.
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x03, geometryShader ? 2 : 0);
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x03, 0);
    GPUCMD_AddMaskedWrite(GPUREG_VSH_COM_MODE, 0x01, geometryShader ? 1 : 0);

    if (vertexShader) {
        GLASS_uploadShaderBinary(vertexShader);
        GPUCMD_AddWrite(GPUREG_VSH_ENTRYPOINT, 0x7FFF0000 | (vertexShader->codeEntrypoint & 0xFFFF));
        GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_MASK, 0x03, vertexShader->outMask);

        // Set vertex shader outmap number.
        GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_TOTAL1, 0x01, vertexShader->outTotal - 1);
        GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_TOTAL2, 0x01, vertexShader->outTotal - 1);
    }

    if (geometryShader) {
        GLASS_uploadShaderBinary(geometryShader);
        GPUCMD_AddWrite(GPUREG_GSH_ENTRYPOINT, 0x7FFF0000 | (geometryShader->codeEntrypoint & 0xFFFF));
        GPUCMD_AddMaskedWrite(GPUREG_GSH_OUTMAP_MASK, 0x01, geometryShader->outMask);
    }

    // Handle outmaps.
    u16 mergedOutTotal = 0;
    u32 mergedOutClock = 0;
    u32 mergedOutSems[7];
    bool useTexcoords = false;

    if (vertexShader && geometryShader && (geometryShader->flags & SHADER_FLAG_MERGE_OUTMAPS)) {
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
        useTexcoords = (vertexShader->flags & SHADER_FLAG_USE_TEXCOORDS) | (geometryShader->flags & SHADER_FLAG_USE_TEXCOORDS);
    } else {
        const ShaderInfo* mainShader = geometryShader ? geometryShader : vertexShader;
        if (mainShader) {
            mergedOutTotal = mainShader->outTotal;
            memcpy(mergedOutSems, mainShader->outSems, 7 * sizeof(u32));
            mergedOutClock = mainShader->outClock;
            useTexcoords = mainShader->flags & SHADER_FLAG_USE_TEXCOORDS;
        }
    }

    if (mergedOutTotal) {
        GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x01, mergedOutTotal - 1);
        GPUCMD_AddMaskedWrite(GPUREG_SH_OUTMAP_TOTAL, 0x01, mergedOutTotal);
        GPUCMD_AddIncrementalWrites(GPUREG_SH_OUTMAP_O0, mergedOutSems, 7);
        GPUCMD_AddMaskedWrite(GPUREG_SH_OUTATTR_MODE, 0x01, useTexcoords ? 1 : 0);
        GPUCMD_AddWrite(GPUREG_SH_OUTATTR_CLOCK, mergedOutClock);
    }

    // TODO: configure geostage.
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x0A, 0);
    GPUCMD_AddWrite(GPUREG_GSH_MISC0, 0);
    GPUCMD_AddWrite(GPUREG_GSH_MISC1, 0);
    GPUCMD_AddWrite(GPUREG_GSH_INPUTBUFFER_CONFIG, 0xA0000000);
}

static INLINE void GLASS_uploadBoolUniformMask(const ShaderInfo* shader, u16 mask) {
    const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_BOOLUNIFORM : GPUREG_VSH_BOOLUNIFORM;
    GPUCMD_AddWrite(reg, 0x7FFF0000 | mask);
}

static INLINE void GLASS_uploadConstIntUniforms(const ShaderInfo* shader) {
    const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    for (size_t i = 0; i < 4; ++i) {
        if (!((shader->constIntMask >> i) & 1))
            continue;

        GPUCMD_AddWrite(reg + i, shader->constIntData[i]);
    }
}

static INLINE void GLASS_uploadConstFloatUniforms(const ShaderInfo* shader) {
    const u32 idReg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    for (size_t i = 0; i < shader->numOfConstFloatUniforms; ++i) {
        const ConstFloatInfo* uni = &shader->constFloatUniforms[i];
        GPUCMD_AddWrite(idReg, uni->ID);
        GPUCMD_AddIncrementalWrites(dataReg, uni->data, 3);
    }
}

void GLASS_gpu_uploadConstUniforms(const ShaderInfo* shader) {
    ASSERT(shader);
    GLASS_uploadBoolUniformMask(shader, shader->constBoolMask);
    GLASS_uploadConstIntUniforms(shader);
    GLASS_uploadConstFloatUniforms(shader);
}

static INLINE void GLASS_uploadIntUniform(const ShaderInfo* shader, UniformInfo* info) {
    const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_INTUNIFORM_I0 : GPUREG_VSH_INTUNIFORM_I0;

    if (info->count == 1) {
        GPUCMD_AddWrite(reg + info->ID, info->data.value);
    } else {
        GPUCMD_AddIncrementalWrites(reg + info->ID, info->data.values, info->count);
    }
}

static INLINE void GLASS_uploadFloatUniform(const ShaderInfo* shader, UniformInfo* info) {
    const u32 idReg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_CONFIG : GPUREG_VSH_FLOATUNIFORM_CONFIG;
    const u32 dataReg = (shader->flags & SHADER_FLAG_GEOMETRY) ? GPUREG_GSH_FLOATUNIFORM_DATA : GPUREG_VSH_FLOATUNIFORM_DATA;

    // ID is automatically incremented after each write.
    GPUCMD_AddWrite(idReg, info->ID);

    for (size_t i = 0; i < info->count; ++i)
        GPUCMD_AddIncrementalWrites(dataReg, &info->data.values[i * 3], 3);
}

void GLASS_gpu_uploadUniforms(ShaderInfo* shader) {
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
                GLASS_uploadIntUniform(shader, uni);
                break;
            case GLASS_UNI_FLOAT:
                GLASS_uploadFloatUniform(shader, uni);
                break;
            default:
                UNREACHABLE("Invalid uniform type!");
        }

        uni->dirty = false;
    }

    if (uploadBool)
        GLASS_uploadBoolUniformMask(shader, boolMask);
}

static int GLASS_compareAttribOffsets(const void *a, const void *b) {
    const u32 v0 = ((u32*)a)[1];
    const u32 v1 = ((u32*)b)[1];
    if (v0 < v1) return -1;
    if (v0 > v1) return 1;
    return 0;
}

// We want to know which buffers to use, which components are applied to it, in which order, etc.
static void GLASS_extractAttribBuffersInfo(const AttributeInfo* attribs, AttributeBuffer* attribBuffers) {
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];

        if (!(attrib->flags & ATTRIB_FLAG_ENABLED) || (attrib->flags & ATTRIB_FLAG_FIXED))
            continue;

        // Find attribute buffer.
        size_t firstEmpty = GLASS_NUM_ATTRIB_BUFFERS;
        bool found = false;
        for (size_t i = 0; i < GLASS_NUM_ATTRIB_BUFFERS; ++i) {
            AttributeBuffer* attribBuffer = &attribBuffers[i];
            if (attribBuffer->physAddr == attrib->physAddr) {
                ASSERT(attribBuffer->stride == attrib->stride);
                ASSERT(attribBuffer->numComponents < GLASS_MAX_ENABLED_ATTRIBS);
                attribBuffer->components[attribBuffer->numComponents] = regId;
                attribBuffer->componentOffsets[attribBuffer->numComponents++] = attrib->physOffset;
                found = true;
                break;
            }

            if (!attribBuffer->physAddr && firstEmpty == GLASS_NUM_ATTRIB_BUFFERS)
                firstEmpty = i;
        }

        // Initialize if not done yet.
        if (!found && firstEmpty != GLASS_NUM_ATTRIB_BUFFERS) {
            AttributeBuffer* attribBuffer = &attribBuffers[firstEmpty];
            attribBuffer->physAddr = attrib->physAddr;
            attribBuffer->stride = attrib->stride;
            attribBuffer->numComponents = 1;
            attribBuffer->components[0] = regId;
            attribBuffer->componentOffsets[0] = attrib->physOffset;
            found = true;
        }

        ASSERT(found);
    }

    // Sort each buffer components.
    for (size_t i = 0; i < GLASS_NUM_ATTRIB_BUFFERS; ++i) {
        AttributeBuffer* attribBuffer = &attribBuffers[i];
        if (!attribBuffer->physAddr)
            continue;

        u32 buffer[GLASS_MAX_ENABLED_ATTRIBS * 2];
        for (size_t j = 0; j < attribBuffer->numComponents; ++j) {
            buffer[j * 2] = attribBuffer->components[j];
            buffer[(j * 2) + 1] = attribBuffer->componentOffsets[j];
        }

        qsort(buffer, attribBuffer->numComponents, sizeof(u32) * 2, GLASS_compareAttribOffsets);

        // We don't need the offsets anymore.
        for (size_t j = 0; j < attribBuffer->numComponents; ++j)
            attribBuffer->components[j] = buffer[j * 2];
    }
}

void GLASS_gpu_uploadAttributes(const AttributeInfo* attribs) {
    ASSERT(attribs);

    // Extract attrib buffers info.
    // Each buffer comes with a list of input registers, ordered by their offset.
    AttributeBuffer attribBuffers[GLASS_NUM_ATTRIB_BUFFERS] = {};
    GLASS_extractAttribBuffersInfo(attribs, attribBuffers);

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
        if (!(attrib->flags & ATTRIB_FLAG_ENABLED))
            continue;

        if (!(attrib->flags & ATTRIB_FLAG_FIXED)) {
            // Set buffer params.
            const GPU_FORMATS attribType = GLASS_utility_getAttribType(attrib->type);

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
    GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFERS_FORMAT_LOW, format, 2);
    GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0x0B, 0xA0000000 | (attribCount - 1));
    GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, attribCount - 1);

    // Map each vertex attribute to an input register.
    GPUCMD_AddIncrementalWrites(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, permutation, 2);

    // Set buffers base.
    GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC, PHYSICAL_LINEAR_BASE >> 3);

    // Step 2: setup fixed attributes.
    // This must be set after initial configuration.
    for (size_t regId = 0; regId < GLASS_NUM_ATTRIB_REGS; ++regId) {
        const AttributeInfo* attrib = &attribs[regId];
        if (!(attrib->flags & ATTRIB_FLAG_ENABLED))
            continue;

        if (attrib->flags & ATTRIB_FLAG_FIXED) {
            u32 packed[3];
            GLASS_utility_packFloatVector(attrib->components, packed);
            GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_INDEX, regTable[regId]);
            GPUCMD_AddIncrementalWrites(GPUREG_FIXEDATTRIB_DATA0, packed, 3);
        }
    }

    // Step 3: setup attribute buffers.
    for (size_t i = 0; i < GLASS_NUM_ATTRIB_BUFFERS; ++i) {
        u32 params[3] = {};
        AttributeBuffer* attribBuffer = &attribBuffers[i];

        if (attribBuffer->physAddr) {
            // Calculate permutation.
            u32 permutation[2] = {};

            // Basically resolve every input register to its mapped vertex attribute.
            for (size_t j = 0; j < attribBuffer->numComponents; ++j) {
                const u32 regId = attribBuffer->components[j];
                const u32 attribIndex = regTable[regId];
                if (j < 8) {
                    permutation[0] = (attribIndex << (j * 4));
                } else {
                    permutation[1] = (attribIndex << (j * 4));
                }
            }

            params[0] = attribBuffer->physAddr - PHYSICAL_LINEAR_BASE;
            params[1] = permutation[0];
            params[2] = permutation[1] | ((attribBuffer->stride & 0xFF) << 16) | ((attribBuffer->numComponents & 0x0F) << 28);
        }

        GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFER0_OFFSET + (i * 0x03), params, 3);
    }
}

void GLASS_gpu_setCombiners(const CombinerInfo* combiners) {
    ASSERT(combiners);

    const size_t offsets[GLASS_NUM_COMBINER_STAGES] = {
        GPUREG_TEXENV0_SOURCE, GPUREG_TEXENV1_SOURCE, GPUREG_TEXENV2_SOURCE,
        GPUREG_TEXENV3_SOURCE, GPUREG_TEXENV4_SOURCE, GPUREG_TEXENV5_SOURCE,
    };

    for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; ++i) {
        u32 params[5];
        const CombinerInfo* combiner = &combiners[i];

        params[0] = GLASS_utility_getCombinerSrc(combiner->rgbSrc[0]);
        params[0] |= (GLASS_utility_getCombinerSrc(combiner->rgbSrc[1]) << 4);
        params[0] |= (GLASS_utility_getCombinerSrc(combiner->rgbSrc[2]) << 8);
        params[0] |= (GLASS_utility_getCombinerSrc(combiner->alphaSrc[0]) << 16);
        params[0] |= (GLASS_utility_getCombinerSrc(combiner->alphaSrc[1]) << 20);
        params[0] |= (GLASS_utility_getCombinerSrc(combiner->alphaSrc[2]) << 24);
        params[1] = GLASS_utility_getCombinerOpRGB(combiner->rgbOp[0]);
        params[1] |= (GLASS_utility_getCombinerOpRGB(combiner->rgbOp[1]) << 4);
        params[1] |= (GLASS_utility_getCombinerOpRGB(combiner->rgbOp[2]) << 8);
        params[1] |= (GLASS_utility_getCombinerOpAlpha(combiner->alphaOp[0]) << 12);
        params[1] |= (GLASS_utility_getCombinerOpAlpha(combiner->alphaOp[1]) << 16);
        params[1] |= (GLASS_utility_getCombinerOpAlpha(combiner->alphaOp[2]) << 20);
        params[2] = GLASS_utility_getCombinerFunc(combiner->rgbFunc);
        params[2] |= (GLASS_utility_getCombinerFunc(combiner->alphaFunc) << 16);
        params[3] = combiner->color;
        params[4] = GLASS_utility_getCombinerScale(combiner->rgbScale);
        params[4] |= (GLASS_utility_getCombinerScale(combiner->alphaScale) << 16);

        GPUCMD_AddIncrementalWrites(offsets[i], params, 5);
    }
}

void GLASS_gpu_setFragOp(GLenum fragMode, bool blendMode) {
    GPU_FRAGOPMODE gpuFragMode;

    switch (fragMode) {
        case GL_FRAGOP_MODE_DEFAULT_PICA:
            gpuFragMode = GPU_FRAGOPMODE_GL;
            break;
        case GL_FRAGOP_MODE_SHADOW_PICA:
            gpuFragMode = GPU_FRAGOPMODE_SHADOW;
            break;
        case GL_FRAGOP_MODE_GAS_PICA:
            gpuFragMode = GPU_FRAGOPMODE_GAS_ACC;
            break;
        default:
            UNREACHABLE("Invalid fragment mode!");
    }

    GPUCMD_AddMaskedWrite(GPUREG_COLOR_OPERATION, 0x07, 0xE40000 | (blendMode ? 0x100 : 0x0) | gpuFragMode);
}

void GLASS_gpu_setColorDepthMask(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha, bool writeDepth, bool depthTest, GLenum depthFunc) {
    u32 value = ((writeRed ? 0x0100 : 0x00) | (writeGreen ? 0x0200 : 0x00) | (writeBlue ? 0x0400 : 0x00) | (writeAlpha ? 0x0800 : 0x00));

    if (depthTest)
        value |= (GLASS_utility_getTestFunc(depthFunc) << 4) | (writeDepth ? 0x1000 : 0x00) | 1;

    GPUCMD_AddMaskedWrite(GPUREG_DEPTH_COLOR_MASK, 0x03, value);
}

void GLASS_gpu_setDepthMap(bool enabled, GLclampf nearVal, GLclampf farVal, GLfloat units, GLenum depthFormat) {
    float offset = 0.0f;
    ASSERT(nearVal >= 0.0f && nearVal <= 1.0f);
    ASSERT(farVal >= 0.0f && farVal <= 1.0f);

    switch (depthFormat) {
        case GL_DEPTH_COMPONENT16:
            offset = (units / 65535.0f);
            break;
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH24_STENCIL8:
            offset = (units / 16777215.0f);
            break;
    }

    GPUCMD_AddMaskedWrite(GPUREG_DEPTHMAP_ENABLE, 0x01, enabled ? 1 : 0);
    GPUCMD_AddWrite(GPUREG_DEPTHMAP_SCALE, f32tof24(nearVal - farVal));
    GPUCMD_AddWrite(GPUREG_DEPTHMAP_OFFSET, f32tof24(nearVal + offset));
}

void GLASS_gpu_setEarlyDepthTest(bool enabled) {
    GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x01, enabled ? 1 : 0);
    GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST2, 0x01, enabled ? 1 : 0);
}

void GLASS_gpu_setEarlyDepthFunc(GPU_EARLYDEPTHFUNC func) { GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_FUNC, 0x01, func); }

void GLASS_gpu_setEarlyDepthClear(GLclampf value) {
    ASSERT(value >= 0.0f && value <= 1.0f);
    GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_DATA, 0x07, 0xFFFFFF * value);
}

void GLASS_gpu_clearEarlyDepthBuffer(void) { GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1); }

void GLASS_gpu_setStencilTest(bool enabled, GLenum func, GLint ref, GLuint mask, GLuint writeMask) {
    u32 value = enabled ? 1 : 0;
    if (enabled) {
        value |= (GLASS_utility_getTestFunc(func) << 4);
        value |= ((u8)writeMask << 8);
        value |= ((int8_t)ref << 16);
        value |= ((u8)mask << 24);
    }

    GPUCMD_AddWrite(GPUREG_STENCIL_TEST, value);
}

void GLASS_gpu_setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) { GPUCMD_AddMaskedWrite(GPUREG_STENCIL_OP, 0x03, GLASS_utility_getStencilOp(sfail) | (GLASS_utility_getStencilOp(dpfail) << 4) | (GLASS_utility_getStencilOp(dppass) << 8)); }

void GLASS_gpu_setCullFace(bool enabled, GLenum cullFace, GLenum frontFace) {
    // Essentially:
    // - set FRONT-CCW for FRONT-CCW/BACK-CW;
    // - set BACK-CCW in all other cases.
    const GPU_CULLMODE mode = ((cullFace == GL_FRONT) != (frontFace == GL_CCW)) ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
    GPUCMD_AddMaskedWrite(GPUREG_FACECULLING_CONFIG, 0x01, enabled ? mode : GPU_CULL_NONE);
}

void GLASS_gpu_setAlphaTest(bool enabled, GLenum func, GLclampf ref) {
    ASSERT(ref >= 0.0f && ref <= 1.0f);
    u32 value = enabled ? 1 : 0;
    if (enabled) {
        value |= (GLASS_utility_getTestFunc(func) << 4);
        value |= ((u8)(ref * 0xFF) << 8);
    }

    GPUCMD_AddMaskedWrite(GPUREG_FRAGOP_ALPHA_TEST, 0x03, value);
}

void GLASS_gpu_setBlendFunc(GLenum rgbEq, GLenum alphaEq, GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha) {
    const GPU_BLENDEQUATION gpuRGBEq = GLASS_utility_getBlendEq(rgbEq);
    const GPU_BLENDEQUATION gpuAlphaEq = GLASS_utility_getBlendEq(alphaEq);
    const GPU_BLENDFACTOR gpuSrcColor = GLASS_utility_getBlendFactor(srcColor);
    const GPU_BLENDFACTOR gpuDstColor = GLASS_utility_getBlendFactor(dstColor);
    const GPU_BLENDFACTOR gpuSrcAlpha = GLASS_utility_getBlendFactor(srcAlpha);
    const GPU_BLENDFACTOR gpuDstAlpha = GLASS_utility_getBlendFactor(dstAlpha);
    GPUCMD_AddWrite(GPUREG_BLEND_FUNC, (gpuDstAlpha << 28) | (gpuSrcAlpha << 24) | (gpuDstColor << 20) | (gpuSrcColor << 16) | (gpuAlphaEq << 8) | gpuRGBEq);
}

void GLASS_gpu_setBlendColor(u32 color) { GPUCMD_AddWrite(GPUREG_BLEND_COLOR, color); }
void GLASS_gpu_setLogicOp(GLenum op) { GPUCMD_AddMaskedWrite(GPUREG_LOGIC_OP, 0x01, GLASS_utility_getLogicOp(op)); }

void GLASS_gpu_drawArrays(GLenum mode, GLint first, GLsizei count) {
    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2, GLASS_utility_getDrawPrimitive(mode));
    GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
    GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000);
    GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);
    GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, first);
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 1);
    GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
    GPUCMD_AddWrite(GPUREG_DRAWARRAYS, 1);
    GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 0);
    GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);
}

void GLASS_gpu_drawElements(GLenum mode, GLsizei count, GLenum type, u32 physIndices) {
    const GPU_Primitive_t primitive = GLASS_utility_getDrawPrimitive(mode);
    const u32 gpuType = GLASS_utility_getDrawType(type);

    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2, primitive != GPU_TRIANGLES ? primitive : GPU_GEOMETRY_PRIM);

    GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
    GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, (physIndices - PHYSICAL_LINEAR_BASE) | (gpuType << 31));

    GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);
    GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0);

    if (primitive == GPU_TRIANGLES) {
        GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0x100);
        GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0x100);
    }

    GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
    GPUCMD_AddWrite(GPUREG_DRAWELEMENTS, 1);
    GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);

    if (primitive == GPU_TRIANGLES) {
        GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0);
        GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0);
    }

    GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);
    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x08, 0);
}

void GLASS_gpu_setTextureUnits(const TextureUnit* units) {
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

    u32 config = (1u << 16); // Clear cache.

    for (size_t i = 0; i < GLASS_NUM_TEXTURE_UNITS; ++i) {
        const TextureUnit* unit = &units[i];
        if (!unit->dirty)
            continue;

        const TextureInfo* tex = (TextureInfo*)unit->texture;
        if (!tex)
            continue;

        // DEBUG.
        ASSERT(!i);

        // Enable unit.
        config |= (1u << i);

        const bool setupCubeMap = !i && (tex->target == GL_TEXTURE_CUBE_MAP);
        u32 params[10] = {};

        // Setup common info.
        params[0] = tex->borderColor;
        params[1] = ((u32)(tex->width & 0x3FF) << 16) | (tex->height & 0x3FF);

        const GPU_TEXTURE_FILTER_PARAM minFilter = GLASS_utility_getTexFilter(tex->minFilter);
        const GPU_TEXTURE_FILTER_PARAM magFilter = GLASS_utility_getTexFilter(tex->magFilter);
        const GPU_TEXTURE_FILTER_PARAM mipFilter = GLASS_utility_getMipFilter(tex->minFilter);
        const GPU_TEXTURE_WRAP_PARAM wrapS = GLASS_utility_getTexWrap(tex->wrapS);
        const GPU_TEXTURE_WRAP_PARAM wrapT = GLASS_utility_getTexWrap(tex->wrapT);

        params[2] = (GPU_TEXTURE_MIN_FILTER(minFilter) | GPU_TEXTURE_MAG_FILTER(magFilter) | GPU_TEXTURE_MIP_FILTER(mipFilter) | GPU_TEXTURE_WRAP_S(wrapS) | GPU_TEXTURE_WRAP_T(wrapT));
        
        if (false /* ETC1 check */)
            params[2] |= GPU_TEXTURE_ETC1_PARAM;

        if (!i) {
            // TODO
            params[2] |= GPU_TEXTURE_MODE(GPU_TEX_2D);
            // TODO: Shadow
        }

        params[3] = GLASS_utility_f32tofixed13(tex->lodBias) | (((u32)tex->maxLod & 0x0F) << 16) | (((u32)tex->minLod & 0x0F) << 24);

        if (setupCubeMap) {
            // TODO
        } else {
            params[4] = 0; // address
        }

        GPUCMD_AddIncrementalWrites(setupCmds[i], params, setupCubeMap ? 10 : 5); // TODO
        // TODO: Shadow
		GPUCMD_AddWrite(typeCmds[i], 0); // TODO
    }

    GPUCMD_AddWrite(GPUREG_TEXUNIT_CONFIG, config);
}