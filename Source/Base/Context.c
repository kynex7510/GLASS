#include "Base/Context.h"
#include "Platform/GPU.h"

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
        ctx->settings.gpuCmdList.mainBuffer = NULL;
        ctx->settings.gpuCmdList.secondBuffer = NULL;
        ctx->settings.gpuCmdList.capacity = 0;
        ctx->settings.gpuCmdList.offset = 0;
        ctx->settings.vsync = true;
        ctx->settings.horizontalFlip = false;
        ctx->settings.downscale = GLASS_DOWNSCALE_NONE;
    }

    // Platform.
    ctx->flags = 0;
    ctx->lastError = GL_NO_ERROR;
    memset(&ctx->gxQueue, 0, sizeof(ctx->gxQueue));

    GLASS_gpu_allocList(&ctx->settings.gpuCmdList);
    KYGX_BREAK_UNLESS(kygxInit());

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
    ctx->scissorMode = GPU_SCISSOR_DISABLE;
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
    ASSERT(ctx);

    if (ctx == g_Context)
        GLASS_context_bind(NULL);

    GLASS_gx_cleanup(ctx);
    GLASS_gpu_freeList(&ctx->settings.gpuCmdList);
}

CtxCommon* GLASS_context_getCommon(void) {
    ASSERT(g_Context);
    return g_Context;
}

void GLASS_context_bind(CtxCommon* ctx) {
    if (ctx == g_Context)
        return;

    const bool skipUpdate = (g_Context == NULL) && (ctx == g_OldCtx);

    if (g_Context)
        GLASS_gx_unbind(g_Context);

    g_OldCtx = g_Context;
    g_Context = ctx;

    if (g_Context) {
        GLASS_gx_bind(g_Context);

        if (!skipUpdate)
            g_Context->flags = GLASS_CONTEXT_FLAG_ALL;
    }
}

static GLsizei GLASS_renderWidth(CtxCommon* ctx) {
    ASSERT(ctx);

    if (ctx->framebuffer != GLASS_INVALID_OBJECT) {
        const FramebufferInfo* fb = (FramebufferInfo*)ctx->framebuffer;
        const RenderbufferInfo* rb = (RenderbufferInfo*)fb->colorBuffer;
        if (rb)
            return rb->width;
    }

    u16 width;
    gfxGetFramebuffer(ctx->settings.targetScreen, ctx->settings.targetSide, NULL, &width);
    return width;
}

void GLASS_context_flush(void) {
    ASSERT(g_Context);

    // Handle framebuffer.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_FRAMEBUFFER) {
        FramebufferInfo* info = (FramebufferInfo*)g_Context->framebuffer;

        // Flush buffers if required.
        if (g_Context->flags & GLASS_CONTEXT_FLAG_DRAW) {
            GLASS_gpu_flushFramebuffer(&g_Context->settings.gpuCmdList);
            GLASS_gpu_clearEarlyDepthBuffer(&g_Context->settings.gpuCmdList);
            g_Context->flags &= ~(GLASS_CONTEXT_FLAG_DRAW | GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR);
        }

        GLASS_gpu_bindFramebuffer(&g_Context->settings.gpuCmdList, info, g_Context->block32);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_FRAMEBUFFER;
    }

    // Handle draw.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_DRAW) {
        GLASS_gpu_flushFramebuffer(&g_Context->settings.gpuCmdList);
        GLASS_gpu_invalidateFramebuffer(&g_Context->settings.gpuCmdList);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_DRAW;
    }

    // Handle viewport.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_VIEWPORT) {
        // Account for rotated screens.
        const GLsizei x = (GLASS_renderWidth(g_Context) - (g_Context->viewportX + g_Context->viewportW));
        GLASS_gpu_setViewport(&g_Context->settings.gpuCmdList, x, g_Context->viewportY, g_Context->viewportW, g_Context->viewportH);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_VIEWPORT;
    }

    // Handle scissor.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_SCISSOR) {
        // Account for rotated screens.
        const GLsizei x = (GLASS_renderWidth(g_Context) - (g_Context->scissorX + g_Context->scissorW));
        GLASS_gpu_setScissorTest(&g_Context->settings.gpuCmdList, g_Context->scissorMode, x, g_Context->scissorY, g_Context->scissorW, g_Context->scissorH);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_SCISSOR;
    }

    // Handle program.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_PROGRAM) {
        ProgramInfo* pinfo = (ProgramInfo*)g_Context->currentProgram;

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

            GLASS_gpu_bindShaders(&g_Context->settings.gpuCmdList, vs, gs);

            if (vs)
                GLASS_gpu_uploadConstUniforms(&g_Context->settings.gpuCmdList, vs);

            if (gs)
                GLASS_gpu_uploadConstUniforms(&g_Context->settings.gpuCmdList, gs);
        }

        g_Context->flags &= ~GLASS_CONTEXT_FLAG_PROGRAM;
    }

    // Handle uniforms.
    if (GLASS_OBJ_IS_PROGRAM(g_Context->currentProgram)) {
        ProgramInfo* pinfo = (ProgramInfo*)g_Context->currentProgram;
        ShaderInfo* vs = (ShaderInfo*)pinfo->linkedVertex;
        ShaderInfo* gs = (ShaderInfo*)pinfo->linkedGeometry;

        if (vs)
            GLASS_gpu_uploadUniforms(&g_Context->settings.gpuCmdList, vs);

        if (gs)
            GLASS_gpu_uploadUniforms(&g_Context->settings.gpuCmdList, gs);
    }

    // Handle attributes.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_ATTRIBS) {
        GLASS_gpu_uploadAttributes(&g_Context->settings.gpuCmdList, g_Context->attribs);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_ATTRIBS;
    }

    // Handle fragop.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_FRAGOP) {
        GLASS_gpu_setFragOp(&g_Context->settings.gpuCmdList, g_Context->fragMode, g_Context->blendMode);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_FRAGOP;
    }

    // Handle color and depth masks.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_COLOR_DEPTH) {
        GLASS_gpu_setColorDepthMask(&g_Context->settings.gpuCmdList, g_Context->writeRed, g_Context->writeGreen, g_Context->writeBlue, g_Context->writeAlpha, g_Context->writeDepth, g_Context->depthTest, g_Context->depthFunc);
        // TODO: check gas!!!!
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_COLOR_DEPTH;
    }

    // Handle depth map.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_DEPTHMAP) {
        GLenum depthFormat = GL_DEPTH_COMPONENT16;
        if (g_Context->framebuffer != GLASS_INVALID_OBJECT) {
            const FramebufferInfo* fb = (FramebufferInfo*)g_Context->framebuffer;
            if (fb->depthBuffer != GLASS_INVALID_OBJECT) {
                const RenderbufferInfo* db = (RenderbufferInfo*)fb->depthBuffer;
                depthFormat = db->format;
            }
        }

        GLASS_gpu_setDepthMap(&g_Context->settings.gpuCmdList, g_Context->polygonOffset, g_Context->depthNear, g_Context->depthFar, g_Context->polygonUnits, depthFormat);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_DEPTHMAP;
    }

    // Handle early depth.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_EARLY_DEPTH) {
        GLASS_gpu_setEarlyDepthTest(&g_Context->settings.gpuCmdList, g_Context->earlyDepthTest);
        GLASS_gpu_setEarlyDepthFunc(&g_Context->settings.gpuCmdList, g_Context->earlyDepthFunc);
        GLASS_gpu_setEarlyDepthClear(&g_Context->settings.gpuCmdList, g_Context->clearEarlyDepth);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_EARLY_DEPTH;
    }

    // Handle early depth clear.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR) {
        GLASS_gpu_clearEarlyDepthBuffer(&g_Context->settings.gpuCmdList);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_EARLY_DEPTH_CLEAR;
    }

    // Handle stencil.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_STENCIL) {
        GLASS_gpu_setStencilTest(&g_Context->settings.gpuCmdList, g_Context->stencilTest, g_Context->stencilFunc, g_Context->stencilRef, g_Context->stencilMask, g_Context->stencilWriteMask);
        GLASS_gpu_setStencilOp(&g_Context->settings.gpuCmdList, g_Context->stencilFail, g_Context->stencilDepthFail, g_Context->stencilPass);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_STENCIL;
    }

    // Handle cull face.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_CULL_FACE) {
        GLASS_gpu_setCullFace(&g_Context->settings.gpuCmdList, g_Context->cullFace, g_Context->cullFaceMode, g_Context->frontFaceMode);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_CULL_FACE;
    }

    // Handle alpha.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_ALPHA) {
        GLASS_gpu_setAlphaTest(&g_Context->settings.gpuCmdList, g_Context->alphaTest, g_Context->alphaFunc, g_Context->alphaRef);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_ALPHA;
    }

    // Handle blend & logic op.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_BLEND) {
        GLASS_gpu_setBlendFunc(&g_Context->settings.gpuCmdList, g_Context->blendEqRGB, g_Context->blendEqAlpha, g_Context->blendSrcRGB, g_Context->blendDstRGB, g_Context->blendSrcAlpha, g_Context->blendDstAlpha);
        GLASS_gpu_setBlendColor(&g_Context->settings.gpuCmdList, g_Context->blendColor);
        GLASS_gpu_setLogicOp(&g_Context->settings.gpuCmdList, g_Context->logicOp);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_BLEND;
    }

    // Handle textures.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_TEXTURE) {
        GLASS_gpu_setTextureUnits(&g_Context->settings.gpuCmdList, g_Context->textureUnits);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_TEXTURE;
    }

    // Handle combiners.
    if (g_Context->flags & GLASS_CONTEXT_FLAG_COMBINERS) {
        GLASS_gpu_setCombiners(&g_Context->settings.gpuCmdList, g_Context->combiners);
        g_Context->flags &= ~GLASS_CONTEXT_FLAG_COMBINERS;
    }
}

#ifndef GLASS_NO_MERCY
void GLASS_context_setError(GLenum error) {
    ASSERT(g_Context);
    if (g_Context->lastError == GL_NO_ERROR)
        g_Context->lastError = error;
}
#endif // GLASS_NO_MERCY