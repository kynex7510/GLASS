/**
 * Shaders have a refcount. It's set to 1 at creation.
 * It's incremented:
 * - each time it's attached to a program;
 * - when linking programs (to be linked).
 *
 * It's decremented:
 * - when explicitly deleted (only the first time);
 * - each time it's detached from a program;
 * - when linking programs (already linked).
 *
 * Shared shader data also have a refcount. That is
 * incremented at shader initialization, and decremented
 * when a shader linked to it is unlinked/deleted.
 *
 * This should be evenly distributed.
 */
#include "Base/Context.h"
#include "Base/Math.h"

#include <string.h> // strlen, memset, memcpy

#define DVLB_MIN_SIZE 0x08
#define DVLB_MAGIC "\x44\x56\x4C\x42"

#define DVLP_MIN_SIZE 0x28
#define DVLP_MAGIC "\x44\x56\x4C\x50"

#define DVLE_MIN_SIZE 0x40
#define DVLE_MAGIC "\x44\x56\x4C\x45"

typedef struct {
    u32 numOfDVLEs;  // Number of DVLE entries.
    u32 DVLETable[]; // DVLE entries.
} DVLB;

typedef struct {
    u16 type; //  Constant type.
    u16 ID;   // Constant ID.
    union {
        u32 boolUniform; // Bool uniform value.
        u32 intUniform;  // Int uniform value.
        struct {
            u32 x; // Float24 uniform X component.
            u32 y; // Float24 uniform Y component.
            u32 z; // Float24 uniform Z component.
            u32 w; // Float24 uniform W component.
        } floatUniform;
    } data;
} DVLEConstEntry;

typedef struct{
    u16 type;
    u16 regID;
    u8 mask;
    u8 _unk[3];
} DVLEOutEntry;

typedef struct{
    u32 symbolOffset;
    u16 startReg;
    u16 endReg;
} DVLEUniformEntry;

typedef struct {
    bool isGeometry;                  // This DVLE is for a geometry shader.
    bool mergeOutmaps;                // Merge shader outmaps (geometry only).
    u32 entrypoint;                   // Code entrypoint.
    GPUGeoShaderMode gsMode;          // Geometry shader mode.
    DVLEConstEntry* constUniforms;    // Constant uniform table.
    u32 numOfConstUniforms;           // Size of constant uniform table.
    DVLEOutEntry* outRegs;            // Output register table.
    u32 numOfOutRegs;                 // Size of output register table.
    DVLEUniformEntry* activeUniforms; // Uniform table.
    u32 numOfActiveUniforms;          // Size of uniform table.
    char* symbolTable;                // Symbol table.
    u32 sizeOfSymbolTable;            // Size of symbol table.
} DVLEInfo;

static void freeUniformData(ShaderInfo* shader) {
    KYGX_ASSERT(shader);

    for (size_t i = 0; i < shader->numOfActiveUniforms; ++i) {
        UniformInfo* uni = &shader->activeUniforms[i];
        if ((uni->type == GLASS_UNI_FLOAT) || ((uni->type == GLASS_UNI_INT) && (uni->count > 1)))
            glassHeapFree(uni->data.values);
    }

    glassHeapFree(shader->constFloatUniforms);
    glassHeapFree(shader->activeAttribs);
    glassHeapFree(shader->activeUniforms);
}

static inline void decSharedDataRefc(SharedShaderData* sharedData) {
    KYGX_ASSERT(sharedData);

    if (sharedData->refc)
        --sharedData->refc;

    if (!sharedData->refc)
        glassHeapFree(sharedData);
}

static inline void decShaderRefc(ShaderInfo* shader) {
    KYGX_ASSERT(shader);

    if (shader->refc)
        --shader->refc;

    if (!shader->refc) {
        KYGX_ASSERT(shader->flags & GLASS_SHADER_FLAG_DELETE);

        if (shader->sharedData)
            decSharedDataRefc(shader->sharedData);

        freeUniformData(shader);
        glassHeapFree(shader);
    }
}

static void detachFromProgram(ProgramInfo* pinfo, ShaderInfo* sinfo) {
    KYGX_ASSERT(pinfo);
    KYGX_ASSERT(sinfo);

    const GLuint sobj = (GLuint)sinfo;

    if (sinfo->flags & GLASS_SHADER_FLAG_GEOMETRY) {
        if (pinfo->attachedGeometry != sobj) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        pinfo->attachedGeometry = GLASS_INVALID_OBJECT;
    } else {
        if (pinfo->attachedVertex != sobj) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        pinfo->attachedVertex = GLASS_INVALID_OBJECT;
    }

    decShaderRefc((ShaderInfo*)sinfo);
}

static void freeProgram(ProgramInfo* info) {
    KYGX_ASSERT(info);
    KYGX_ASSERT(info->flags & GLASS_PROGRAM_FLAG_DELETE);

    if (GLASS_OBJ_IS_SHADER(info->attachedVertex))
        decShaderRefc((ShaderInfo*)info->attachedVertex);

    if (GLASS_OBJ_IS_SHADER(info->attachedGeometry))
        decShaderRefc((ShaderInfo*)info->attachedGeometry);

    if (GLASS_OBJ_IS_SHADER(info->linkedVertex))
        decShaderRefc((ShaderInfo*)info->linkedVertex);

    if (GLASS_OBJ_IS_SHADER(info->linkedGeometry))
        decShaderRefc((ShaderInfo*)info->linkedGeometry);

    glassHeapFree(info);
}

static inline size_t numActiveUniforms(const ProgramInfo* info) {
    KYGX_ASSERT(info);

    size_t numOfActiveUniforms = 0;

    if (GLASS_OBJ_IS_SHADER(info->linkedVertex)) {
        const ShaderInfo* vshad = (const ShaderInfo*)info->linkedVertex;
        numOfActiveUniforms = vshad->numOfActiveUniforms;
    }

    if (GLASS_OBJ_IS_SHADER(info->linkedGeometry)) {
        const ShaderInfo* gshad = (const ShaderInfo*)info->linkedGeometry;
        numOfActiveUniforms += gshad->numOfActiveUniforms;
    }

    return numOfActiveUniforms;
}

static inline size_t lenActiveUniforms(const ProgramInfo* info) {
    KYGX_ASSERT(info);

    size_t lenOfActiveUniforms = 0;

    if (GLASS_OBJ_IS_SHADER(info->linkedVertex)) {
        const ShaderInfo* vshad = (const ShaderInfo*)info->linkedVertex;
        if (vshad->activeUniformsMaxLen > lenOfActiveUniforms)
            lenOfActiveUniforms = vshad->activeUniformsMaxLen;

        if (GLASS_OBJ_IS_SHADER(info->linkedGeometry)) {
            const ShaderInfo* gshad = (const ShaderInfo*)info->linkedGeometry;
            if (gshad->activeUniformsMaxLen > lenOfActiveUniforms)
                lenOfActiveUniforms = gshad->activeUniformsMaxLen;
        }
    }

    return lenOfActiveUniforms;
}

// TODO: verify the behaviour with geo shaders. Do we need to read from them?
static inline size_t numActiveAttribs(const ProgramInfo* info) {
    KYGX_ASSERT(info);

    size_t numOfActiveAttribs = 0;

    if (GLASS_OBJ_IS_SHADER(info->linkedVertex)) {
        const ShaderInfo* vshad = (const ShaderInfo*)info->linkedVertex;
        numOfActiveAttribs = vshad->numOfActiveAttribs;
    }

    if (GLASS_OBJ_IS_SHADER(info->linkedGeometry)) {
        const ShaderInfo* gshad = (const ShaderInfo*)info->linkedGeometry;
        numOfActiveAttribs += gshad->numOfActiveAttribs;
    }

    return numOfActiveAttribs;
}

static inline size_t lenActiveAttribs(const ProgramInfo* info) {
    KYGX_ASSERT(info);

    size_t lenOfActiveAttribs = 0;

    if (GLASS_OBJ_IS_SHADER(info->linkedVertex)) {
        const ShaderInfo* vshad = (const ShaderInfo*)info->linkedVertex;
        if (vshad->activeAttribsMaxLen > lenOfActiveAttribs)
            lenOfActiveAttribs = vshad->activeAttribsMaxLen;

        if (GLASS_OBJ_IS_SHADER(info->linkedGeometry)) {
            const ShaderInfo* gshad = (const ShaderInfo*)info->linkedGeometry;
            if (gshad->activeAttribsMaxLen > lenOfActiveAttribs)
                lenOfActiveAttribs = gshad->activeAttribsMaxLen;
        }
    }

    return lenOfActiveAttribs;
}

static size_t lookupShader(const GLuint* shaders, size_t maxShaders, size_t index, bool isGeometry) {
    KYGX_ASSERT(shaders);

    for (size_t i = (index + 1); i < maxShaders; ++i) {
        GLuint name = shaders[i];

        // Force failure if one of the shaders is invalid.
        if (!GLASS_OBJ_IS_SHADER(name))
            break;

        const ShaderInfo* shader = (const ShaderInfo*)name;
        if (((shader->flags & GLASS_SHADER_FLAG_GEOMETRY) == GLASS_SHADER_FLAG_GEOMETRY) == isGeometry)
            return i;
    }

    return index;
}

static DVLB* parseDVLB(const u8* data, size_t size) {
    KYGX_ASSERT(data);
    KYGX_ASSERT(size > DVLB_MIN_SIZE);
    KYGX_ASSERT(memcmp(data, DVLB_MAGIC, sizeof(DVLB_MAGIC) - 1) == 0);

    // Read number of DVLEs.
    u32 numOfDVLEs = 0;
    memcpy(&numOfDVLEs, data + 0x04, sizeof(u32));
    KYGX_ASSERT((DVLB_MIN_SIZE + (numOfDVLEs * 4)) <= size);

    // Allocate DVLB.
    DVLB* dvlb = (DVLB*)glassHeapAlloc( sizeof(DVLB) + (numOfDVLEs * 4));

    if (dvlb) {
        dvlb->numOfDVLEs = numOfDVLEs;

        // Fill table with offsets.
        for (size_t i = 0; i < numOfDVLEs; ++i) {
            dvlb->DVLETable[i] = *(u32*)(data + DVLB_MIN_SIZE + (4 * i));
            KYGX_ASSERT(dvlb->DVLETable[i] <= size);
            // Relocation.
            dvlb->DVLETable[i] += (u32)data;
        }
    }

    return dvlb;
}

static SharedShaderData* parseDVLP(const u8* data, size_t size) {
    KYGX_ASSERT(data);
    KYGX_ASSERT(size > DVLP_MIN_SIZE);
    KYGX_ASSERT(memcmp(data, DVLP_MAGIC, sizeof(DVLP_MAGIC) - 1) == 0);

    // Read offsets.
    u32 offsetToBlob = 0;
    u32 offsetToOpDescs = 0;
    memcpy(&offsetToBlob, data + 0x08, sizeof(u32));
    memcpy(&offsetToOpDescs, data + 0x10, sizeof(u32));
    KYGX_ASSERT(offsetToBlob < size);
    KYGX_ASSERT(offsetToOpDescs < size);

    // Read num params.
    u32 numOfCodeWords = 0;
    u32 numOfOpDescs = 0;
    memcpy(&numOfCodeWords, data + 0x0C, sizeof(u32));
    memcpy(&numOfOpDescs, data + 0x14, sizeof(u32));

    KYGX_ASSERT(numOfCodeWords <= 512);
    KYGX_ASSERT(numOfOpDescs <= 128);
    KYGX_ASSERT((offsetToBlob + (numOfCodeWords * 4)) <= size);
    KYGX_ASSERT((offsetToOpDescs + (numOfOpDescs * 8)) <= size);

    // Allocate data.
    SharedShaderData* sharedData = (SharedShaderData*)glassHeapAlloc(sizeof(SharedShaderData) + (numOfCodeWords * sizeof(u32)) + (numOfOpDescs * sizeof(u32)));

    if (sharedData) {
        sharedData->refc = 0;
        sharedData->binaryCode = (u32*)((u8*)sharedData + sizeof(SharedShaderData));
        sharedData->numOfCodeWords = numOfCodeWords;
        sharedData->opDescs = sharedData->binaryCode + sharedData->numOfCodeWords;
        sharedData->numOfOpDescs = numOfOpDescs;

        // Read binary code.
        memcpy(sharedData->binaryCode, data + offsetToBlob, sharedData->numOfCodeWords * sizeof(u32));

        // Read op descs.
        for (size_t i = 0; i < sharedData->numOfOpDescs; ++i)
            sharedData->opDescs[i] = ((u32*)(data + offsetToOpDescs))[i * 2];
    }

    return sharedData;
}

static void getDVLEInfo(const u8* data, size_t size, DVLEInfo* out) {
    KYGX_ASSERT(data);
    KYGX_ASSERT(out);
    KYGX_ASSERT(size > DVLE_MIN_SIZE);
    KYGX_ASSERT(memcmp(data, DVLE_MAGIC, sizeof(DVLE_MAGIC) - 1) == 0);

    // Get info.
    u8 flags = 0;
    u8 mergeOutmaps = 0;
    u8 gsMode = 0;
    u32 offsetToConstTable = 0;
    u32 offsetToOutTable = 0;
    u32 offsetToUniformTable = 0;
    u32 offsetToSymbolTable = 0;
    memcpy(&flags, data + 0x06, sizeof(u8));
    memcpy(&mergeOutmaps, data + 0x07, sizeof(u8));
    memcpy(&out->entrypoint, data + 0x08, sizeof(u32));
    memcpy(&gsMode, data + 0x14, sizeof(u8));
    memcpy(&offsetToConstTable, data + 0x18, sizeof(u32));
    memcpy(&out->numOfConstUniforms, data + 0x1C, sizeof(u32));
    memcpy(&offsetToOutTable, data + 0x28, sizeof(u32));
    memcpy(&out->numOfOutRegs, data + 0x2C, sizeof(u32));
    memcpy(&offsetToUniformTable, data + 0x30, sizeof(u32));
    memcpy(&out->numOfActiveUniforms, data + 0x34, sizeof(u32));
    memcpy(&offsetToSymbolTable, data + 0x38, sizeof(u32));
    memcpy(&out->sizeOfSymbolTable, data + 0x3C, sizeof(u32));
    KYGX_ASSERT(offsetToConstTable < size);
    KYGX_ASSERT(offsetToOutTable < size);
    KYGX_ASSERT(offsetToUniformTable < size);
    KYGX_ASSERT(offsetToSymbolTable < size);
    KYGX_ASSERT((offsetToConstTable + (out->numOfConstUniforms * 20)) <= size);
    KYGX_ASSERT((offsetToOutTable + (out->numOfOutRegs * 8)) <= size);
    KYGX_ASSERT((offsetToUniformTable + (out->numOfActiveUniforms * 8)) <= size);
    KYGX_ASSERT((offsetToSymbolTable + out->sizeOfSymbolTable) <= size);

    // Handle geometry shader.
    switch (flags) {
        case 0x00:
            out->isGeometry = false;
            break;
        case 0x01:
            out->isGeometry = true;
            break;
        default:
            KYGX_UNREACHABLE("Unknown DVLE flags value!");
    }

    // Handle merge outmaps.
    if (mergeOutmaps & 1) {
        KYGX_ASSERT(out->isGeometry);
        out->mergeOutmaps = true;
    } else {
        out->mergeOutmaps = false;
    }

    // Handle geometry mode.
    if (out->isGeometry) {
        switch (gsMode) {
            case 0x00:
                out->gsMode = GEOSHADERMODE_POINT;
                break;
            case 0x01:
                out->gsMode = GEOSHADERMODE_VARIABLE;
                break;
            case 0x02:
                out->gsMode = GEOSHADERMODE_FIXED;
                break;
            default:
                KYGX_UNREACHABLE("Unknown DVLE geometry shader mode!");
        }
    }

    // Set table pointers.
    out->constUniforms = (DVLEConstEntry*)(data + offsetToConstTable);
    out->outRegs = (DVLEOutEntry*)(data + offsetToOutTable);
    out->activeUniforms = (DVLEUniformEntry*)(data + offsetToUniformTable);
    out->symbolTable = (char*)(data + offsetToSymbolTable);
}

static void generateOutmaps(const DVLEInfo* info, ShaderInfo* out) {
    KYGX_ASSERT(info);
    KYGX_ASSERT(out);

    bool useTexcoords = false;
    out->outMask = 0;
    out->outTotal = 0;
    out->outClock = 0;
    memset(out->outSems, 0x1F, sizeof(out->outSems));

    for (size_t i = 0; i < info->numOfOutRegs; ++i) {
        const DVLEOutEntry* entry = &info->outRegs[i];
        u8 sem = 0x1F;
        size_t maxSem = 0;

        // Set output register.
        if (!(out->outMask & (1u << entry->regID))) {
            out->outMask |= (1u << entry->regID);
            ++out->outTotal;
        }

        // Get register semantics.
        switch (entry->type) {
            case OUTPUTREGTYPE_POSITION:
                sem = 0x00;
                maxSem = 4;
                break;
            case OUTPUTREGTYPE_NORMALQUAT:
                sem = 0x04;
                maxSem = 4;
                out->outClock |= (1u << 24);
                break;
            case OUTPUTREGTYPE_COLOR:
                sem = 0x08;
                maxSem = 4;
                out->outClock |= (1u << 1);
                break;
            case OUTPUTREGTYPE_TEXCOORD0:
                sem = 0x0C;
                maxSem = 2;
                out->outClock |= (1u << 8);
                useTexcoords = true;
                break;
            case OUTPUTREGTYPE_TEXCOORD0W:
                sem = 0x10;
                maxSem = 1;
                out->outClock |= (1u << 16);
                useTexcoords = true;
                break;
            case OUTPUTREGTYPE_TEXCOORD1:
                sem = 0x0E;
                maxSem = 2;
                out->outClock |= (1u << 9);
                useTexcoords = true;
                break;
            case OUTPUTREGTYPE_TEXCOORD2:
                sem = 0x16;
                maxSem = 2;
                out->outClock |= (1u << 10);
                useTexcoords = true;
                break;
            case OUTPUTREGTYPE_VIEW:
                sem = 0x12;
                maxSem = 3;
                out->outClock |= (1u << 24);
                break;
            case OUTPUTREGTYPE_DUMMY:
                continue;
            default:
                KYGX_UNREACHABLE("Unknown output register type!");
        }

        // Set register semantics.
        for (size_t i = 0, curSem = 0; i < 4 && curSem < maxSem; ++i) {
            if (entry->mask & (1u << i)) {
                out->outSems[entry->regID] &= ~(0xFF << (i * 8));
                out->outSems[entry->regID] |= ((sem++) << (i * 8));

                // Check for position.z clock.
                ++curSem;
                if ((entry->type == OUTPUTREGTYPE_POSITION) && (curSem == 3))
                    out->outClock |= (1u << 0);
            }
        }
    }

    if (useTexcoords) {
        out->flags |= GLASS_SHADER_FLAG_USE_TEXCOORDS;
    } else {
        out->flags &= ~GLASS_SHADER_FLAG_USE_TEXCOORDS;
    }
}

static bool loadUniforms(const DVLEInfo* info, ShaderInfo* out) {
    KYGX_ASSERT(info);
    KYGX_ASSERT(out);

    // Cleanup.
    freeUniformData(out);
    out->constBoolMask = 0;
    memset(out->constIntData, 0, sizeof(out->constIntData));
    out->constIntMask = 0;
    out->constFloatUniforms = NULL;
    out->numOfConstFloatUniforms = 0;
    out->activeUniforms = NULL;
    out->numOfActiveUniforms = 0;
    out->activeUniformsMaxLen = 0;
    out->activeAttribs = NULL;
    out->numOfActiveAttribs = 0;
    out->activeAttribsMaxLen = 0;

    // Setup constant uniforms.
    size_t numOfConstFloatUniforms = 0;
    for (size_t i = 0; i < info->numOfConstUniforms; ++i) {
        const DVLEConstEntry* constEntry = &info->constUniforms[i];

        switch (constEntry->type) {
            case GLASS_UNI_BOOL:
            KYGX_ASSERT(constEntry->ID < GLASS_NUM_BOOL_UNIFORMS);
            if (constEntry->data.boolUniform) {
                out->constBoolMask |= (1u << constEntry->ID);
            }
            break;
        case GLASS_UNI_INT:
            KYGX_ASSERT(constEntry->ID < GLASS_NUM_INT_UNIFORMS);
            out->constIntData[constEntry->ID] = constEntry->data.intUniform;
            out->constIntMask |= (1u << constEntry->ID);
            break;
        case GLASS_UNI_FLOAT:
            KYGX_ASSERT(constEntry->ID < GLASS_NUM_FLOAT_UNIFORMS);
            ++numOfConstFloatUniforms;
            break;
        default:
            KYGX_UNREACHABLE("Unknown const uniform type!");
        }
    }

    out->constFloatUniforms = (ConstFloatInfo*)glassHeapAlloc(sizeof(ConstFloatInfo) * numOfConstFloatUniforms);
    if (!out->constFloatUniforms)
        return false;

    out->numOfConstFloatUniforms = numOfConstFloatUniforms;
    numOfConstFloatUniforms = 0;

    for (size_t i = 0; i < info->numOfConstUniforms; ++i) {
        const DVLEConstEntry* constEntry = &info->constUniforms[i];
        if (constEntry->type != GLASS_UNI_FLOAT)
            continue;

        const float components[] = {
            GLASS_math_f24tof32(constEntry->data.floatUniform.x),
            GLASS_math_f24tof32(constEntry->data.floatUniform.y),
            GLASS_math_f24tof32(constEntry->data.floatUniform.z),
            GLASS_math_f24tof32(constEntry->data.floatUniform.w)
        };

        ConstFloatInfo* uni = &out->constFloatUniforms[numOfConstFloatUniforms++];
        uni->ID = constEntry->ID;
        GLASS_math_packFloatVector(components, uni->data);
    }

    KYGX_ASSERT(numOfConstFloatUniforms == out->numOfConstFloatUniforms);

    // Find number of active attributes.
    // Shader binaries do not differentiate between active uniforms and active attributes.
    for (size_t i = 0; i < info->numOfActiveUniforms; ++i) {
        const DVLEUniformEntry* entry = &info->activeUniforms[i];
        if ((entry->startReg >= 0x00) && (entry->startReg <= 0x0F)) {
            ++out->numOfActiveAttribs;

            // Update max symbol length.
            const size_t symLen = strlen((char*)(out->symbolTable + entry->symbolOffset)) + 1;
            if (symLen > out->activeAttribsMaxLen)
                out->activeAttribsMaxLen = symLen;
        }
    }

    // Setup active attributes.
    out->activeAttribs = (ActiveAttribInfo*)glassHeapAlloc(sizeof(ActiveAttribInfo) * out->numOfActiveAttribs);
    if (!out->activeAttribs)
        return false;

    size_t index = 0;
    for (size_t i = 0; i < info->numOfActiveUniforms; ++i) {
        const DVLEUniformEntry* entry = &info->activeUniforms[i];
        if (entry->startReg >= 0x10)
            continue;

        KYGX_ASSERT(entry->startReg == entry->endReg);
        ActiveAttribInfo* attrib = &out->activeAttribs[index++];
        attrib->ID = entry->startReg;
        attrib->symbol = out->symbolTable + entry->symbolOffset;
    }

    KYGX_ASSERT(index == out->numOfActiveAttribs);

    // Setup active uniforms.
    out->numOfActiveUniforms = info->numOfActiveUniforms - out->numOfActiveAttribs;
    out->activeUniforms = (UniformInfo*)glassHeapAlloc(sizeof(UniformInfo) * out->numOfActiveUniforms);
    if (!out->activeUniforms)
        return false;

    index = 0;
    for (size_t i = 0; i < info->numOfActiveUniforms; ++i) {
        const DVLEUniformEntry* entry = &info->activeUniforms[i];

        // Skip attributes.
        if ((entry->startReg >= 0x00) && (entry->startReg <= 0x0F))
            continue;

        UniformInfo* uni = &out->activeUniforms[index++];
        uni->ID = entry->startReg;
        uni->count = (entry->endReg + 1) - entry->startReg;
        uni->symbol = out->symbolTable + entry->symbolOffset;

        // Update max symbol length.
        const size_t symLen = strlen(uni->symbol) + 1;
        if (symLen > out->activeUniformsMaxLen)
            out->activeUniformsMaxLen = symLen;

        // Handle bool.
        if ((entry->startReg >= 0x78) && (entry->startReg <= 0x87)) {
            KYGX_ASSERT(entry->endReg <= 0x87);
            uni->ID -= 0x78;
            uni->type = GLASS_UNI_BOOL;
            uni->data.mask = 0;
            continue;
        }

        // Handle int.
        if ((entry->startReg >= 0x70) && (entry->startReg <= 0x73)) {
            KYGX_ASSERT(entry->endReg <= 0x73);
            uni->ID -= 0x70;
            uni->type = GLASS_UNI_INT;

            if (uni->count > 1) {
                uni->data.values = (u32*)glassHeapAlloc(sizeof(u32) * uni->count);
                if (!uni->data.values)
                    return false;
            } else {
                uni->data.value = 0;
            }

            continue;
        }

        // Handle float.
        if ((entry->startReg >= 0x10) && (entry->startReg <= 0x6F)) {
            KYGX_ASSERT(entry->endReg <= 0x6F);
            uni->ID -= 0x10;
            uni->type = GLASS_UNI_FLOAT;
            uni->data.values = (u32*)glassHeapAlloc((sizeof(u32) * 3) * uni->count);
            if (!uni->data.values)
                return false;

            continue;
        }

        KYGX_UNREACHABLE("Unknown uniform type!");
    }

    KYGX_ASSERT(index == out->numOfActiveUniforms);
    return true;
}

void glAttachShader(GLuint program, GLuint shader) {
    if (!GLASS_OBJ_IS_PROGRAM(program) || !GLASS_OBJ_IS_SHADER(shader)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* pinfo = (ProgramInfo*)program;
    ShaderInfo* sinfo = (ShaderInfo*)shader;

    // Attach shader to program.
    if (sinfo->flags & GLASS_SHADER_FLAG_GEOMETRY) {
        if (GLASS_OBJ_IS_SHADER(pinfo->attachedGeometry)) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        pinfo->attachedGeometry = shader;
    } else {
        if (GLASS_OBJ_IS_SHADER(pinfo->attachedVertex)) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            return;
        }

        pinfo->attachedVertex = shader;
    }

    // Increment shader refcount.
    ++sinfo->refc;
}

GLuint glCreateProgram(void) {
    GLuint name = GLASS_createObject(GLASS_PROGRAM_TYPE);
    if (GLASS_OBJ_IS_PROGRAM(name)) {
        ProgramInfo* info = (ProgramInfo*)name;
        info->attachedVertex = GLASS_INVALID_OBJECT;
        info->linkedVertex = GLASS_INVALID_OBJECT;
        info->attachedGeometry = GLASS_INVALID_OBJECT;
        info->linkedGeometry = GLASS_INVALID_OBJECT;
        info->flags = 0;
        return name;
    }

    GLASS_context_setError(GL_OUT_OF_MEMORY);
    return GLASS_INVALID_OBJECT;
}

GLuint glCreateShader(GLenum shaderType) {
    u16 flags = 0;

    switch (shaderType) {
        case GL_VERTEX_SHADER:
            break;
        case GL_GEOMETRY_SHADER_PICA:
            flags = GLASS_SHADER_FLAG_GEOMETRY;
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
            return GLASS_INVALID_OBJECT;
    }

    // Create shader.
    GLuint name = GLASS_createObject(GLASS_SHADER_TYPE);
    if (GLASS_OBJ_IS_SHADER(name)) {
        ShaderInfo* info = (ShaderInfo*)name;
        info->flags = flags;
        info->refc = 1;
        return name;
    }

    GLASS_context_setError(GL_OUT_OF_MEMORY);
    return GLASS_INVALID_OBJECT;
}

void glDeleteProgram(GLuint program) {
    // A value of 0 is silently ignored.
    if (program == GLASS_INVALID_OBJECT)
        return;

    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();
    ProgramInfo* info = (ProgramInfo*)program;

    // Flag for deletion.
    if (!(info->flags & GLASS_PROGRAM_FLAG_DELETE)) {
        info->flags |= GLASS_PROGRAM_FLAG_DELETE;
        if (ctx->currentProgram != program)
            freeProgram(info);
    }
}

void glDeleteShader(GLuint shader) {
    // A value of 0 is silently ignored.
    if (shader == GLASS_INVALID_OBJECT)
        return;

    if (!GLASS_OBJ_IS_SHADER(shader)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    ShaderInfo* info = (ShaderInfo*)shader;

    // Flag for deletion.
    if (!(info->flags & GLASS_SHADER_FLAG_DELETE)) {
        info->flags |= GLASS_SHADER_FLAG_DELETE;
        decShaderRefc(info);
    }
}

void glDetachShader(GLuint program, GLuint shader) {
    if (!GLASS_OBJ_IS_PROGRAM(program) || !GLASS_OBJ_IS_SHADER(shader)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* pinfo = (ProgramInfo*)program;
    ShaderInfo* sinfo = (ShaderInfo*)shader;
    detachFromProgram(pinfo, sinfo);
}

void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders) {
    KYGX_ASSERT(shaders);

    GLsizei shaderCount = 0;

    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    if (maxCount < 0) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

    ProgramInfo* info = (ProgramInfo*)program;

    // Get shaders.
    if (shaderCount < maxCount) {
        if (GLASS_OBJ_IS_SHADER(info->attachedVertex))
            shaders[shaderCount++] = info->attachedVertex;
    }

    if (shaderCount < maxCount) {
        if (GLASS_OBJ_IS_SHADER(info->attachedGeometry))
            shaders[shaderCount++] = info->attachedGeometry;
    }

    // Get count.
    if (count)
        *count = shaderCount;
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    KYGX_ASSERT(params);

    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* info = (ProgramInfo*)program;

    // Get parameter.
    switch (pname) {
        case GL_DELETE_STATUS:
            *params = (info->flags & GLASS_PROGRAM_FLAG_DELETE) ? GL_TRUE : GL_FALSE;
            break;
        case GL_LINK_STATUS:
            *params = (info->flags & GLASS_PROGRAM_FLAG_LINK_FAILED) ? GL_FALSE : GL_TRUE;
            break;
        case GL_VALIDATE_STATUS:
            *params = GL_TRUE;
            break;
        case GL_INFO_LOG_LENGTH:
            *params = 0;
            break;
        case GL_ATTACHED_SHADERS:
            if (GLASS_OBJ_IS_SHADER(info->attachedVertex) && GLASS_OBJ_IS_SHADER(info->attachedGeometry)) {
               *params = 2;
            } else if (GLASS_OBJ_IS_SHADER(info->attachedVertex) || GLASS_OBJ_IS_SHADER(info->attachedGeometry)) {
               *params = 1;
            }  else {
               *params = 0;
            }
            break;
        case GL_ACTIVE_UNIFORMS:
            *params = numActiveUniforms(info);
            break;
        case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            *params = lenActiveUniforms(info);
            break;
        case GL_ACTIVE_ATTRIBUTES:
            *params = numActiveAttribs(info);
            break;
        case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
            *params = lenActiveAttribs(info);
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
    }
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    KYGX_ASSERT(params);

    if (!GLASS_OBJ_IS_SHADER(shader)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ShaderInfo* info = (ShaderInfo*)shader;

    // Get parameter.
    switch (pname) {
        case GL_SHADER_TYPE:
            *params = (info->flags & GLASS_SHADER_FLAG_GEOMETRY) ? GL_GEOMETRY_SHADER_PICA : GL_VERTEX_SHADER;
            break;
        case GL_DELETE_STATUS:
            *params = (info->flags & GLASS_SHADER_FLAG_DELETE) ? GL_TRUE : GL_FALSE;
            break;
        case GL_COMPILE_STATUS:
        case GL_INFO_LOG_LENGTH:
        case GL_SHADER_SOURCE_LENGTH:
            GLASS_context_setError(GL_INVALID_OPERATION);
            break;
        default:
            GLASS_context_setError(GL_INVALID_ENUM);
    }
}

GLboolean glIsProgram(GLuint program) { return GLASS_OBJ_IS_PROGRAM(program) ? GL_TRUE : GL_FALSE; }
GLboolean glIsShader(GLuint shader) { return GLASS_OBJ_IS_SHADER(shader) ? GL_TRUE : GL_FALSE; }

void glLinkProgram(GLuint program) {
    if (!GLASS_OBJ_IS_PROGRAM(program)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    ProgramInfo* pinfo = (ProgramInfo*)program;

    // Check for attached vertex shader (required).
    if (!GLASS_OBJ_IS_SHADER(pinfo->attachedVertex)) {
        pinfo->flags |= GLASS_PROGRAM_FLAG_LINK_FAILED;
        return;
    }

    // Check if vertex shader is new.
    if (pinfo->attachedVertex != pinfo->linkedVertex) {
        ShaderInfo* vsinfo = (ShaderInfo*)pinfo->attachedVertex;

        // Check load.
        if (!vsinfo->sharedData) {
            pinfo->flags |= GLASS_PROGRAM_FLAG_LINK_FAILED;
            return;
        }

        // Unlink old vertex shader.
        if (GLASS_OBJ_IS_SHADER(pinfo->linkedVertex))
            decShaderRefc((ShaderInfo*)pinfo->linkedVertex);

        // Link new vertex shader.
        pinfo->flags |= GLASS_PROGRAM_FLAG_UPDATE_VERTEX;
        pinfo->linkedVertex = pinfo->attachedVertex;
        ++vsinfo->refc;
    }

    // Check for attached geometry shader (optional).
    if (GLASS_OBJ_IS_SHADER(pinfo->attachedGeometry) && (pinfo->attachedGeometry != pinfo->linkedGeometry)) {
        ShaderInfo* gsinfo = (ShaderInfo*)pinfo->attachedGeometry;

        // Check load.
        if (!gsinfo->sharedData) {
            pinfo->flags |= GLASS_PROGRAM_FLAG_LINK_FAILED;
            return;
        }

        // Unlink old geometry shader.
        if (GLASS_OBJ_IS_SHADER(pinfo->linkedGeometry))
            decShaderRefc((ShaderInfo*)pinfo->linkedGeometry);

        // Link new geometry shader.
        pinfo->flags |= GLASS_PROGRAM_FLAG_UPDATE_GEOMETRY;
        pinfo->linkedGeometry = pinfo->attachedGeometry;
        ++gsinfo->refc;
    }

    pinfo->flags &= ~GLASS_PROGRAM_FLAG_LINK_FAILED;
}

void glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length) {
    KYGX_ASSERT(shaders);
    KYGX_ASSERT(binary);

    const u8* data = (u8*)binary;
    const size_t size = length;
    size_t lastVertexIdx = (size_t)-1;
    size_t lastGeometryIdx = (size_t)-1;
    DVLB* dvlb = NULL;
    SharedShaderData* sharedData = NULL;

    if (binaryformat != GL_SHADER_BINARY_PICA) {
        GLASS_context_setError(GL_INVALID_ENUM);
        return;
    }

    if ((n < 0) || (length < 0)) {
        GLASS_context_setError(GL_INVALID_VALUE);
        return;
    }

  // Do nothing if no shaders.
    if (!n)
        goto glShaderBinary_skip;

  // Parse DVLB.
    dvlb = parseDVLB(data, size);
    if (!dvlb) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        goto glShaderBinary_skip;
    }

    // Parse DVLP.
    const size_t dvlbSize = DVLB_MIN_SIZE + (dvlb->numOfDVLEs * sizeof(u32));
    sharedData = parseDVLP(data + dvlbSize, size - dvlbSize);
    if (!sharedData) {
        GLASS_context_setError(GL_OUT_OF_MEMORY);
        goto glShaderBinary_skip;
    }

    // Handle DVLEs.
    for (size_t i = 0; i < dvlb->numOfDVLEs; ++i) {
        const u8* pointerToDVLE = (u8*)dvlb->DVLETable[i];
        const size_t maxSize = size - (size_t)(pointerToDVLE - data);

        // Lookup shader.
        DVLEInfo info;
        getDVLEInfo(pointerToDVLE, maxSize, &info);
        const size_t index = lookupShader(shaders, n, (info.isGeometry ? lastGeometryIdx : lastVertexIdx), info.isGeometry);

        if (index == (info.isGeometry ? lastGeometryIdx : lastVertexIdx)) {
            GLASS_context_setError(GL_INVALID_OPERATION);
            goto glShaderBinary_skip;
        }

        // Build shader.
        ShaderInfo* shader = (ShaderInfo*)shaders[index];
        if (info.mergeOutmaps) {
            shader->flags |= GLASS_SHADER_FLAG_MERGE_OUTMAPS;
        } else {
            shader->flags &= ~GLASS_SHADER_FLAG_MERGE_OUTMAPS;
        }

        shader->codeEntrypoint = info.entrypoint;

        if (info.isGeometry)
            shader->gsMode = info.gsMode;

        generateOutmaps(&info, shader);

        // Make copy of symbol table.
        glassHeapFree(shader->symbolTable);
        shader->symbolTable = NULL;
        shader->sizeOfSymbolTable = 0;

        shader->symbolTable = (char*)glassHeapAlloc(info.sizeOfSymbolTable);
        if (!shader->symbolTable) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            goto glShaderBinary_skip;
        }

        memcpy(shader->symbolTable, info.symbolTable, info.sizeOfSymbolTable);
        shader->sizeOfSymbolTable = info.sizeOfSymbolTable;

        // Load uniforms, can only fail for memory issues.
        if (!loadUniforms(&info, shader)) {
            GLASS_context_setError(GL_OUT_OF_MEMORY);
            goto glShaderBinary_skip;
        }

        // Set shared data.
        if (shader->sharedData)
            decSharedDataRefc(shader->sharedData);

        shader->sharedData = sharedData;
        ++sharedData->refc;

        // Increment index.
        if (info.isGeometry) {
            lastGeometryIdx = index;
        } else {
            lastVertexIdx = index;
        }
    }

glShaderBinary_skip:
    // Free resources.
    if (sharedData && !sharedData->refc)
        glassHeapFree(sharedData);

    glassHeapFree(dvlb);
}

void glUseProgram(GLuint program) {
    if (!GLASS_OBJ_IS_PROGRAM(program) && (program != GLASS_INVALID_OBJECT)) {
        GLASS_context_setError(GL_INVALID_OPERATION);
        return;
    }

    CtxCommon* ctx = GLASS_context_getBound();

    // Check if already in use.
    if (ctx->currentProgram != program) {
        // Check if program is linked.
        if (GLASS_OBJ_IS_PROGRAM(program)) {
        ProgramInfo* info = (ProgramInfo*)program;
            if (info->flags & GLASS_PROGRAM_FLAG_LINK_FAILED) {
                GLASS_context_setError(GL_INVALID_VALUE);
                return;
            }
        }

        // Remove program.
        if (GLASS_OBJ_IS_PROGRAM(ctx->currentProgram))
            freeProgram((ProgramInfo*)ctx->currentProgram);

        // Set program.
        ctx->currentProgram = program;
        ctx->flags |= GLASS_CONTEXT_FLAG_PROGRAM;
    }
}

void glCompileShader(GLuint shader) { GLASS_context_setError(GL_INVALID_OPERATION); }

void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    KYGX_ASSERT(infoLog);

    if (length)
        *length = 0;

    *infoLog = '\0';
}

void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    KYGX_ASSERT(infoLog);

    if (length)
        *length = 0;

    *infoLog = '\0';
}

void glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint* range, GLint* precision) { GLASS_context_setError(GL_INVALID_OPERATION); }
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source) { GLASS_context_setError(GL_INVALID_OPERATION); }
void glReleaseShaderCompiler(void) { GLASS_context_setError(GL_INVALID_OPERATION); }
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) { GLASS_context_setError(GL_INVALID_OPERATION); }
void glValidateProgram(GLuint program) {}