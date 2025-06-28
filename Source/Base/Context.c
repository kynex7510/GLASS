#include <KYGX/Wrappers/FlushCacheRegions.h>
#include <KYGX/Wrappers/ProcessCommandList.h>

#include "Base/Context.h"
#include "Platform/GPU.h"
#include "Platform/GFX.h"

#include <string.h> // memset

static CtxCommon* g_Context = NULL;
static CtxCommon* g_OldCtx = NULL;

void GLASS_context_initCommon(CtxCommon* ctx, const GLASSInitParams* initParams, const GLASSSettings* settings) {
    KYGX_ASSERT(ctx);

    ctx->initParams.version = initParams->version;
    ctx->initParams.flushAllLinearMem = initParams->flushAllLinearMem;

    if (settings) {
        memcpy(&ctx->settings, settings, sizeof(GLASSSettings));
    } else {
        ctx->settings.targetScreen = GLASS_SCREEN_TOP;
        ctx->settings.targetSide = GLASS_SIDE_LEFT;
        ctx->settings.GPUCmdList.mainBuffer = NULL;
        ctx->settings.GPUCmdList.secondBuffer = NULL;
        ctx->settings.GPUCmdList.capacity = 0;
        ctx->settings.GPUCmdList.offset = 0;
        ctx->settings.vsync = true;
        ctx->settings.horizontalFlip = false;
        ctx->settings.downscale = GLASS_DOWNSCALE_NONE;
    }

    ctx->flags = 0;
    ctx->lastError = GL_NO_ERROR;

    // Platform.
    GLASS_gpu_allocList(&ctx->settings.GPUCmdList);
    KYGX_BREAK_UNLESS(kygxCmdBufferAlloc(&ctx->GXCmdBuf, 32));

    // Buffers.
    ctx->arrayBuffer = GLASS_INVALID_OBJECT;
    ctx->elementArrayBuffer = GLASS_INVALID_OBJECT;

    // Framebuffer.
    ctx->framebuffer = GLASS_INVALID_OBJECT;
    ctx->renderbuffer = GLASS_INVALID_OBJECT;
    ctx->clearColor = 0;
    ctx->clearDepth = 1.0f;
    ctx->clearStencil = 0;
    ctx->block32 = false;

    // Viewport.
    ctx->viewportX = 0;
    ctx->viewportY = 0;
    ctx->viewportW = 0;
    ctx->viewportH = 0;

    // Scissor.
    ctx->scissorMode = SCISSORMODE_DISABLE;
    ctx->scissorX = 0;
    ctx->scissorY = 0;
    ctx->scissorW = 0;
    ctx->scissorH = 0;

    // Program.
    ctx->currentProgram = GLASS_INVALID_OBJECT;

    // Attributes.
    for (size_t i = 0; i < GLASS_NUM_ATTRIB_REGS; ++i) {
        AttributeInfo* attrib = &ctx->attribs[i];
        attrib->type = GL_FLOAT;
        attrib->count = 4;
        attrib->stride = 0;
        attrib->boundBuffer = GLASS_INVALID_OBJECT;
        attrib->physAddr = 0;
        attrib->bufferOffset = 0;
        attrib->bufferSize = 0;
        attrib->components[0] = 0.0f;
        attrib->components[1] = 0.0f;
        attrib->components[2] = 0.0f;
        attrib->components[3] = 1.0f;
        attrib->sizeOfPrePad = 0;
        attrib->sizeOfPostPad = 0;
        attrib->flags = GLASS_ATTRIB_FLAG_FIXED;
    }

    ctx->numEnabledAttribs = 0;

    // Fragment.
    ctx->fragMode = GL_FRAGOP_MODE_DEFAULT_PICA;
    ctx->flags |= GLASS_CONTEXT_FLAG_FRAGOP;

    // Color, Depth.
    ctx->writeRed = true;
    ctx->writeGreen = true;
    ctx->writeBlue = true;
    ctx->writeAlpha = true;
    ctx->writeDepth = true;
    ctx->depthTest = false;
    ctx->depthFunc = GL_LESS;
    ctx->flags |= GLASS_CONTEXT_FLAG_COLOR_DEPTH;

    // Depth Map.
    ctx->depthNear = 0.0f;
    ctx->depthFar = 1.0f;
    ctx->polygonOffset = false;
    ctx->polygonFactor = 0.0f;
    ctx->polygonUnits = 0.0f;
    ctx->flags |= GLASS_CONTEXT_FLAG_DEPTHMAP;

    // Early Depth.
    ctx->earlyDepthTest = false;
    ctx->clearEarlyDepth = 1.0f;
    ctx->earlyDepthFunc = GL_LESS;
    ctx->flags |= GLASS_CONTEXT_FLAG_EARLY_DEPTH;

    // Stencil.
    ctx->stencilTest = false;
    ctx->stencilFunc = GL_ALWAYS;
    ctx->stencilRef = 0;
    ctx->stencilMask = 0xFFFFFFFF;
    ctx->stencilWriteMask = 0xFFFFFFFF;
    ctx->stencilFail = GL_KEEP;
    ctx->stencilDepthFail = GL_KEEP;
    ctx->stencilPass = GL_KEEP;
    ctx->flags |= GLASS_CONTEXT_FLAG_STENCIL;

    // Cull Face.
    ctx->cullFace = false;
    ctx->cullFaceMode = GL_BACK;
    ctx->frontFaceMode = GL_CCW;
    ctx->flags |= GLASS_CONTEXT_FLAG_CULL_FACE;

    // Alpha.
    ctx->alphaTest = false;
    ctx->alphaFunc = GL_ALWAYS;
    ctx->alphaRef = 0;
    ctx->flags |= GLASS_CONTEXT_FLAG_ALPHA;

    // Blend.
    ctx->blendMode = false;
    ctx->blendColor = 0;
    ctx->blendEqRGB = GL_FUNC_ADD;
    ctx->blendEqAlpha = GL_FUNC_ADD;
    ctx->blendSrcRGB = GL_ONE;
    ctx->blendDstRGB = GL_ZERO;
    ctx->blendSrcAlpha = GL_ONE;
    ctx->blendDstAlpha = GL_ZERO;
    ctx->logicOp = GL_COPY;
    ctx->flags |= GLASS_CONTEXT_FLAG_BLEND;

    // Texture.
    for (size_t i = 0; i < GLASS_NUM_TEX_UNITS; ++i)
        ctx->textureUnits[i] = GLASS_INVALID_OBJECT;

    ctx->activeTextureUnit = 0;
    ctx->flags |= GLASS_CONTEXT_FLAG_TEXTURE;

    // Combiners.
    ctx->combinerStage = 0;
    for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; ++i) {
        CombinerInfo* combiner = &ctx->combiners[i];
        combiner->rgbSrc[0] = !i ? GL_PRIMARY_COLOR : GL_PREVIOUS;
        combiner->rgbSrc[1] = GL_PRIMARY_COLOR;
        combiner->rgbSrc[2] = GL_PRIMARY_COLOR;
        combiner->alphaSrc[0] = !i ? GL_PRIMARY_COLOR : GL_PREVIOUS;
        combiner->alphaSrc[1] = GL_PRIMARY_COLOR;
        combiner->alphaSrc[2] = GL_PRIMARY_COLOR;
        combiner->rgbOp[0] = GL_SRC_COLOR;
        combiner->rgbOp[1] = GL_SRC_COLOR;
        combiner->rgbOp[2] = GL_SRC_COLOR;
        combiner->alphaOp[0] = GL_SRC_ALPHA;
        combiner->alphaOp[1] = GL_SRC_ALPHA;
        combiner->alphaOp[2] = GL_SRC_ALPHA;
        combiner->rgbFunc = GL_REPLACE;
        combiner->alphaFunc = GL_REPLACE;
        combiner->rgbScale = 1.0f;
        combiner->alphaScale = 1.0f;
        combiner->color = 0xFFFFFFFF;
    }

    ctx->flags |= GLASS_CONTEXT_FLAG_COMBINERS;
}

void GLASS_context_cleanupCommon(CtxCommon* ctx) {
    KYGX_ASSERT(ctx);

    if (ctx == g_Context)
        GLASS_context_bind(NULL);

    kygxCmdBufferFree(&ctx->GXCmdBuf);
    GLASS_gpu_freeList(&ctx->settings.GPUCmdList);
}

CtxCommon* GLASS_context_getBound(void) {
    KYGX_ASSERT(g_Context);
    return g_Context;
}

bool GLASS_context_isBound(CtxCommon* ctx) { return g_Context == ctx; }

void GLASS_context_bind(CtxCommon* ctx) {
    if (ctx == g_Context)
        return;

    const bool skipUpdate = (g_Context == NULL) && (ctx == g_OldCtx);

    if (g_Context)
        kygxExchangeCmdBuffer(NULL, false);

    g_OldCtx = g_Context;
    g_Context = ctx;

    if (g_Context) {
        kygxExchangeCmdBuffer(&g_Context->GXCmdBuf, false);

        if (!skipUpdate)
            g_Context->flags = GLASS_CONTEXT_FLAG_ALL;
    }
}

static inline GLsizei renderWidth(CtxCommon* ctx) {
    KYGX_ASSERT(ctx);

    if (ctx->framebuffer != GLASS_INVALID_OBJECT) {
        const FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
        const RenderbufferInfo* rb = (RenderbufferInfo*)fb->colorBuffer;
        if (rb)
            return rb->width;
    }

    u16 width;
    GLASS_gfx_getFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, NULL, &width);
    return width;
}

void GLASS_context_flush(CtxCommon* ctx, bool send) {
    KYGX_ASSERT(ctx);

    // Handle framebuffer.
    if (ctx->flags & GLASS_CONTEXT_FLAG_FRAMEBUFFER) {
        FramebufferInfo* info = (FramebufferInfo*)ctx->framebuffer;

        // Flush buffers if required.
        if (ctx->flags & GLASS_CONTEXT_FLAG_DRAW) {
            GLASS_gpu_flushFramebuffer(&ctx->settings.GPUCmdList);
            GLASS_gpu_clearEarlyDepthBuffer(&ctx->settings.GPUCmdList);
            ctx->flags &= ~(GLASS_CONTEXT_FLAG_DRAW | GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR);
        }

        GLASS_gpu_bindFramebuffer(&ctx->settings.GPUCmdList, info, ctx->block32);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_FRAMEBUFFER;
    }

    // Handle draw.
    if (ctx->flags & GLASS_CONTEXT_FLAG_DRAW) {
        GLASS_gpu_flushFramebuffer(&ctx->settings.GPUCmdList);
        GLASS_gpu_invalidateFramebuffer(&ctx->settings.GPUCmdList);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_DRAW;
    }

    // Handle viewport.
    if (ctx->flags & GLASS_CONTEXT_FLAG_VIEWPORT) {
        // Account for rotated screens.
        const GLsizei x = (renderWidth(ctx) - (ctx->viewportX + ctx->viewportW));
        GLASS_gpu_setViewport(&ctx->settings.GPUCmdList, x, ctx->viewportY, ctx->viewportW, ctx->viewportH);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_VIEWPORT;
    }

    // Handle scissor.
    if (ctx->flags & GLASS_CONTEXT_FLAG_SCISSOR) {
        // Account for rotated screens.
        const GLsizei x = (renderWidth(ctx) - (ctx->scissorX + ctx->scissorW));
        GLASS_gpu_setScissorTest(&ctx->settings.GPUCmdList, ctx->scissorMode, x, ctx->scissorY, ctx->scissorW, ctx->scissorH);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_SCISSOR;
    }

    // Handle program.
    if (ctx->flags & GLASS_CONTEXT_FLAG_PROGRAM) {
        ProgramInfo* pinfo = (ProgramInfo*)ctx->currentProgram;

        if (pinfo) {
            ShaderInfo* vs = NULL;
            ShaderInfo* gs = NULL;

            if (pinfo->flags & GLASS_PROGRAM_FLAG_UPDATE_VERTEX) {
                vs = (ShaderInfo*)pinfo->linkedVertex;
                pinfo->flags &= ~GLASS_PROGRAM_FLAG_UPDATE_VERTEX;
            }

            if (pinfo->flags & GLASS_PROGRAM_FLAG_UPDATE_GEOMETRY) {
                gs = (ShaderInfo*)pinfo->linkedGeometry;
                pinfo->flags &= ~GLASS_PROGRAM_FLAG_UPDATE_GEOMETRY;
            }

            GLASS_gpu_bindShaders(&ctx->settings.GPUCmdList, vs, gs);

            if (vs)
                GLASS_gpu_uploadConstUniforms(&ctx->settings.GPUCmdList, vs);

            if (gs)
                GLASS_gpu_uploadConstUniforms(&ctx->settings.GPUCmdList, gs);
        }

        ctx->flags &= ~GLASS_CONTEXT_FLAG_PROGRAM;
    }

    // Handle uniforms.
    if (GLASS_OBJ_IS_PROGRAM(ctx->currentProgram)) {
        ProgramInfo* pinfo = (ProgramInfo*)ctx->currentProgram;
        ShaderInfo* vs = (ShaderInfo*)pinfo->linkedVertex;
        ShaderInfo* gs = (ShaderInfo*)pinfo->linkedGeometry;

        if (vs)
            GLASS_gpu_uploadUniforms(&ctx->settings.GPUCmdList, vs);

        if (gs)
            GLASS_gpu_uploadUniforms(&ctx->settings.GPUCmdList, gs);
    }

    // Handle attributes.
    if (ctx->flags & GLASS_CONTEXT_FLAG_ATTRIBS) {
        GLASS_gpu_uploadAttributes(&ctx->settings.GPUCmdList, ctx->attribs);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_ATTRIBS;
    }

    // Handle fragop.
    if (ctx->flags & GLASS_CONTEXT_FLAG_FRAGOP) {
        GLASS_gpu_setFragOp(&ctx->settings.GPUCmdList, ctx->fragMode, ctx->blendMode);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_FRAGOP;
    }

    // Handle color and depth masks.
    if (ctx->flags & GLASS_CONTEXT_FLAG_COLOR_DEPTH) {
        GLASS_gpu_setColorDepthMask(&ctx->settings.GPUCmdList, ctx->writeRed, ctx->writeGreen, ctx->writeBlue, ctx->writeAlpha, ctx->writeDepth, ctx->depthTest, ctx->depthFunc);
        // TODO: check gas!!!!
        ctx->flags &= ~GLASS_CONTEXT_FLAG_COLOR_DEPTH;
    }

    // Handle depth map.
    if (ctx->flags & GLASS_CONTEXT_FLAG_DEPTHMAP) {
        GLenum depthFormat = GL_DEPTH_COMPONENT16;
        if (ctx->framebuffer != GLASS_INVALID_OBJECT) {
            const FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
            if (fb->depthBuffer != GLASS_INVALID_OBJECT) {
                const RenderbufferInfo* db = (RenderbufferInfo*)fb->depthBuffer;
                depthFormat = db->format;
            }
        }

        GLASS_gpu_setDepthMap(&ctx->settings.GPUCmdList, ctx->polygonOffset, ctx->depthNear, ctx->depthFar, ctx->polygonUnits, depthFormat);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_DEPTHMAP;
    }

    // Handle early depth.
    if (ctx->flags & GLASS_CONTEXT_FLAG_EARLY_DEPTH) {
        GLASS_gpu_setEarlyDepthTest(&ctx->settings.GPUCmdList, ctx->earlyDepthTest);
        GLASS_gpu_setEarlyDepthFunc(&ctx->settings.GPUCmdList, ctx->earlyDepthFunc);
        GLASS_gpu_setEarlyDepthClear(&ctx->settings.GPUCmdList, ctx->clearEarlyDepth);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_EARLY_DEPTH;
    }

    // Handle early depth clear.
    if (ctx->flags & GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR) {
        GLASS_gpu_clearEarlyDepthBuffer(&ctx->settings.GPUCmdList);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR;
    }

    // Handle stencil.
    if (ctx->flags & GLASS_CONTEXT_FLAG_STENCIL) {
        GLASS_gpu_setStencilTest(&ctx->settings.GPUCmdList, ctx->stencilTest, ctx->stencilFunc, ctx->stencilRef, ctx->stencilMask, ctx->stencilWriteMask);
        GLASS_gpu_setStencilOp(&ctx->settings.GPUCmdList, ctx->stencilFail, ctx->stencilDepthFail, ctx->stencilPass);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_STENCIL;
    }

    // Handle cull face.
    if (ctx->flags & GLASS_CONTEXT_FLAG_CULL_FACE) {
        GLASS_gpu_setCullFace(&ctx->settings.GPUCmdList, ctx->cullFace, ctx->cullFaceMode, ctx->frontFaceMode);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_CULL_FACE;
    }

    // Handle alpha.
    if (ctx->flags & GLASS_CONTEXT_FLAG_ALPHA) {
        GLASS_gpu_setAlphaTest(&ctx->settings.GPUCmdList, ctx->alphaTest, ctx->alphaFunc, ctx->alphaRef);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_ALPHA;
    }

    // Handle blend & logic op.
    if (ctx->flags & GLASS_CONTEXT_FLAG_BLEND) {
        GLASS_gpu_setBlendFunc(&ctx->settings.GPUCmdList, ctx->blendEqRGB, ctx->blendEqAlpha, ctx->blendSrcRGB, ctx->blendDstRGB, ctx->blendSrcAlpha, ctx->blendDstAlpha);
        GLASS_gpu_setBlendColor(&ctx->settings.GPUCmdList, ctx->blendColor);
        GLASS_gpu_setLogicOp(&ctx->settings.GPUCmdList, ctx->logicOp);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_BLEND;
    }

    // Handle textures.
    if (ctx->flags & GLASS_CONTEXT_FLAG_TEXTURE) {
        GLASS_gpu_setTextureUnits(&ctx->settings.GPUCmdList, ctx->textureUnits);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_TEXTURE;
    }

    // Handle combiners.
    if (ctx->flags & GLASS_CONTEXT_FLAG_COMBINERS) {
        GLASS_gpu_setCombiners(&ctx->settings.GPUCmdList, ctx->combiners);
        ctx->flags &= ~GLASS_CONTEXT_FLAG_COMBINERS;
    }

    // Handle send.
    if (send) {
        // Swap GPU command lists.
        void* addr = NULL;
        size_t size = 0;
        if (!GLASS_gpu_swapListBuffers(&ctx->settings.GPUCmdList, &addr, &size))
            return;

        // Flush all linear memory if required.
        if (ctx->initParams.flushAllLinearMem) {
#ifdef KYGX_BAREMETAL
            // TODO
#else
            extern void* __ctru_linear_heap;
            extern u32 __ctru_linear_heap_size;
            void* flushBase = __ctru_linear_heap;
            const size_t flushSize = __ctru_linear_heap_size;
#endif // KYGX_BAREMETAL

            kygxSyncFlushSingleBuffer(flushBase, flushSize);
        }

        // Send GPU commands.
        kygxLock();
        kygxAddProcessCommandList(&ctx->GXCmdBuf, addr, size, false, !ctx->initParams.flushAllLinearMem);
        kygxCmdBufferFinalize(&ctx->GXCmdBuf, NULL, NULL);
        kygxUnlock(true);
    }
}

#ifndef GLASS_NO_MERCY
void GLASS_context_setError(GLenum error) {
    KYGX_ASSERT(g_Context);
    if (g_Context->lastError == GL_NO_ERROR)
        g_Context->lastError = error;
}
#endif // GLASS_NO_MERCY