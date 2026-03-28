/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
GL_SAMPLE_ALPHA_TO_COVERAGE
params returns a single boolean value indicating if the fragment coverage value should be ANDed with a temporary coverage value based on the fragment's alpha value. The initial value is GL_FALSE. See glSampleCoverage.

GL_SAMPLE_BUFFERS
params returns a single integer value indicating the number of sample buffers associated with the currently bound framebuffer. See glSampleCoverage.

GL_SAMPLE_COVERAGE
params returns a single boolean value indicating if the fragment coverage value should be ANDed with a temporary coverage value based on the current sample coverage value. The initial value is GL_FALSE. See glSampleCoverage.

GL_SAMPLE_COVERAGE_INVERT
params returns a single boolean value indicating if the temporary coverage value should be inverted. See glSampleCoverage.

GL_SAMPLE_COVERAGE_VALUE
params returns a single positive floating-point value indicating the current sample coverage value. See glSampleCoverage.

GL_SAMPLES
params returns a single integer value indicating the coverage mask size of the currently bound framebuffer. See glSampleCoverage.
*/

ON_GET(GL_ACTIVE_TEXTURE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GL_TEXTURE0 + ctx->activeTextureUnit)
END_CASE

ON_GET(GL_ALIASED_LINE_WIDTH_RANGE):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(2)
    SET_FLOAT_PARAM(0, 1.0)
    SET_FLOAT_PARAM(1, 1.0)
END_CASE

ON_GET(GL_ALIASED_POINT_SIZE_RANGE):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(2)
    SET_FLOAT_PARAM(0, 1.0)
    SET_FLOAT_PARAM(1, 1.0)
END_CASE

ON_GET(GL_ALPHA_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBColorSize(ctx, GL_RENDERBUFFER_ALPHA_SIZE))
END_CASE

ON_GET(GL_ARRAY_BUFFER_BINDING):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->arrayBuffer)
END_CASE

ON_GET(GL_BLEND):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->blendMode ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_BLEND_COLOR):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(4)
    ENABLE_NORMALIZATION()

    SET_FLOAT_PARAM(0, ((ctx->blendColor >> 24) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(1, ((ctx->blendColor >> 16) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(2, ((ctx->blendColor >> 8) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(3, (ctx->blendColor & 0xFF) / 255.0f)
END_CASE

ON_GET(GL_BLEND_DST_ALPHA):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendDstAlpha)
END_CASE

ON_GET(GL_BLEND_DST_RGB):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendDstRGB)
END_CASE

ON_GET(GL_BLEND_EQUATION_ALPHA):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendEqAlpha)
END_CASE

ON_GET(GL_BLEND_EQUATION_RGB):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendEqRGB)
END_CASE

ON_GET(GL_BLEND_SRC_ALPHA):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendSrcAlpha)
END_CASE

ON_GET(GL_BLEND_SRC_RGB):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->blendSrcRGB)
END_CASE

ON_GET(GL_BLUE_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBColorSize(ctx, GL_RENDERBUFFER_BLUE_SIZE))
END_CASE

ON_GET(GL_COLOR_CLEAR_VALUE):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(4)
    ENABLE_NORMALIZATION()

    SET_FLOAT_PARAM(0, ((ctx->clearColor >> 24) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(1, ((ctx->clearColor >> 16) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(2, ((ctx->clearColor >> 8) & 0xFF) / 255.0f)
    SET_FLOAT_PARAM(3, (ctx->clearColor & 0xFF) / 255.0f)
END_CASE

ON_GET(GL_COLOR_WRITEMASK):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(4)

    SET_BOOL_PARAM(0, ctx->writeRed ? GL_TRUE : GL_FALSE)
    SET_BOOL_PARAM(1, ctx->writeGreen ? GL_TRUE : GL_FALSE)
    SET_BOOL_PARAM(2, ctx->writeBlue ? GL_TRUE : GL_FALSE)
    SET_BOOL_PARAM(3, ctx->writeAlpha ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_COMPRESSED_TEXTURE_FORMATS):
    SET_TYPE(INT)
    // TODO: I'm not sure whether supporting compressed (ETC1) textures with this API.
    SET_NUM_PARAMS(0)
END_CASE

ON_GET(GL_CULL_FACE):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->cullFace ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_CULL_FACE_MODE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->cullFaceMode)
END_CASE

ON_GET(GL_CURRENT_PROGRAM):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->currentProgram)
END_CASE

ON_GET(GL_DEPTH_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBDepthBits(ctx))
END_CASE

ON_GET(GL_DEPTH_CLEAR_VALUE):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(1)
    ENABLE_NORMALIZATION()
    SET_FLOAT_PARAM(0, ctx->clearDepth)
END_CASE

ON_GET(GL_DEPTH_FUNC):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->depthFunc)
END_CASE

ON_GET(GL_DEPTH_RANGE):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(2)
    ENABLE_NORMALIZATION()

    SET_FLOAT_PARAM(0, ctx->depthNear)
    SET_FLOAT_PARAM(1, ctx->depthFar)
END_CASE

ON_GET(GL_DEPTH_TEST):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->depthTest ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_DEPTH_WRITEMASK):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->writeDepth ? GL_TRUE : GL_FALSE)
END_CASE

// ON_GET(GL_DITHER):
    // TODO: params returns a single boolean value indicating whether dithering of fragment colors and indices is enabled.
    // The initial value is GL_TRUE.
// END_CASE

ON_GET(GL_ELEMENT_ARRAY_BUFFER_BINDING):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->elementArrayBuffer)
END_CASE

ON_GET(GL_FRAMEBUFFER_BINDING):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->framebuffer[GLASS_context_getFBIndex(ctx)])
END_CASE

// Extension
ON_GET(GL_FRAMEBUFFER_BINDING_PICA):
    SET_TYPE(INT)
    SET_NUM_PARAMS(2)
    SET_INT_PARAM(0, ctx->framebuffer[0])
    SET_INT_PARAM(1, ctx->framebuffer[1])
END_CASE

ON_GET(GL_FRONT_FACE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->frontFaceMode)
END_CASE

//ON_GET(GL_GENERATE_MIPMAP_HINT):
    // TODO: params returns one value, a symbolic constant indicating the mode of the mipmap generation filtering hint.
    // The initial value is GL_DONT_CARE. See glHint.
//END_CASE

ON_GET(GL_GREEN_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBColorSize(ctx, GL_RENDERBUFFER_GREEN_SIZE))
END_CASE

ON_GET(GL_IMPLEMENTATION_COLOR_READ_FORMAT):
    // TODO: params returns one value, the format chosen by the implementation in which pixels may be read from the color
    // buffer of the currently bound framebuffer in conjunction with GL_IMPLEMENTATION_COLOR_READ_TYPE. In addition to this
    // implementation-dependent format/type pair, format GL_RGBA in conjunction with type GL_UNSIGNED_BYTE is always allowed
    // by every implementation, regardless of the currently bound render surface. See glReadPixels.
END_CASE

ON_GET(GL_IMPLEMENTATION_COLOR_READ_TYPE):
    // TODO: params returns one value, the type chosen by the implementation with which pixels may be read from the color
    // buffer of the currently bound framebuffer in conjunction with GL_IMPLEMENTATION_COLOR_READ_FORMAT. In addition to this
    // implementation-dependent format/type pair, format GL_RGBA in conjunction with type GL_UNSIGNED_BYTE is always allowed by
    // every implementation, regardless of the currently bound render surface. See glReadPixels.
END_CASE

ON_GET(GL_LINE_WIDTH):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(1)
    SET_FLOAT_PARAM(0, 1.0)
END_CASE

//ON_GET(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS):
    // TODO: params returns one value, the maximum supported texture image units that can be used to access texture maps from
    // the vertex shader and the fragment processor combined. If both the vertex shader and the fragment processing stage access
    // the same texture image unit, then that counts as using two texture image units against this limit. The value must be at
    // least 8. See glActiveTexture.
//END_CASE

ON_GET(GL_MAX_CUBE_MAP_TEXTURE_SIZE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GLASS_MAX_TEX_SIZE)
END_CASE

// ON_GET(GL_MAX_FRAGMENT_UNIFORM_VECTORS):
    // TODO: params returns one value, the maximum number of four-element floating-point, integer, or boolean vectors that
    // can be held in uniform variable storage for a fragment shader. The value must be at least 16. See glUniform.
// END_CASE

ON_GET(GL_MAX_RENDERBUFFER_SIZE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GLASS_MAX_FB_WIDTH) // Width and height are the same.
END_CASE

// ON_GET(GL_MAX_TEXTURE_IMAGE_UNITS):
    // TODO: params returns one value, the maximum supported texture image units that can be used to access texture maps from
    // the fragment shader. The value must be at least 8. See glActiveTexture.
// END_CASE

ON_GET(GL_MAX_TEXTURE_SIZE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GLASS_MAX_TEX_SIZE)
END_CASE

// ON_GET(GL_MAX_VARYING_VECTORS):
    // TODO: params returns one value, the maximum number four-element floating-point vectors available for interpolating
    // varying variables used by vertex and fragment shaders. Varying variables declared as matrices or arrays will consume
    // multiple interpolators. The value must be at least 8.
// END_CASE

ON_GET(GL_MAX_VERTEX_ATTRIBS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GLASS_NUM_ATTRIB_REGS)
END_CASE

// ON_GET(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS):
    // TODO: params returns one value, the maximum supported texture image units that can be used to access texture maps from
    // the vertex shader. The value may be 0. See glActiveTexture.
// END_CASE

// ON_GET(GL_MAX_VERTEX_UNIFORM_VECTORS):
    // TODO: params returns one value, the maximum number of four-element floating-point, integer, or boolean vectors that can
    // be held in uniform variable storage for a vertex shader. The value must be at least 128. See glUniform.
// END_CASE

ON_GET(GL_MAX_VIEWPORT_DIMS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(2)
    SET_INT_PARAM(0, GLASS_MAX_VIEWPORT_WIDTH)
    SET_INT_PARAM(1, GLASS_MAX_VIEWPORT_HEIGHT)
END_CASE

ON_GET(GL_NUM_COMPRESSED_TEXTURE_FORMATS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    // TODO: I'm not sure whether supporting compressed (ETC1) textures with this API.
    SET_INT_PARAM(0, 0)
END_CASE

ON_GET(GL_NUM_SHADER_BINARY_FORMATS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, 1)
END_CASE

ON_GET(GL_PACK_ALIGNMENT):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->packAlignment)
END_CASE

ON_GET(GL_POLYGON_OFFSET_FACTOR):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(1)
    SET_FLOAT_PARAM(0, ctx->polygonFactor)
END_CASE

ON_GET(GL_POLYGON_OFFSET_FILL):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->polygonOffset ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_POLYGON_OFFSET_UNITS):
    SET_TYPE(FLOAT)
    SET_NUM_PARAMS(1)
    SET_FLOAT_PARAM(0, ctx->polygonUnits)
END_CASE

ON_GET(GL_RED_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBColorSize(ctx, GL_RENDERBUFFER_RED_SIZE))
END_CASE

ON_GET(GL_RENDERBUFFER_BINDING):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->renderbuffer)
END_CASE

// TODO: GL_SAMPLE*

ON_GET(GL_SCISSOR_BOX):
    SET_TYPE(INT)
    SET_NUM_PARAMS(4)
    
    SET_INT_PARAM(0, ctx->scissorX)
    SET_INT_PARAM(1, ctx->scissorY)
    SET_INT_PARAM(2, ctx->scissorW)
    SET_INT_PARAM(3, ctx->scissorH)
END_CASE

ON_GET(GL_SCISSOR_TEST):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->scissorMode == SCISSORMODE_NORMAL ? GL_TRUE : GL_FALSE)
END_CASE

// Extension
ON_GET(GL_SCISSOR_TEST_INVERTED_PICA):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, ctx->scissorMode == SCISSORMODE_INVERTED ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_SHADER_BINARY_FORMATS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, GL_SHADER_BINARY_PICA)
END_CASE

ON_GET(GL_SHADER_COMPILER):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(0, GL_FALSE)
END_CASE

ON_GET(GL_STENCIL_BACK_FAIL):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilFail)
END_CASE

ON_GET(GL_STENCIL_BACK_FUNC):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilFunc)
END_CASE

ON_GET(GL_STENCIL_BACK_PASS_DEPTH_FAIL):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilDepthFail)
END_CASE

ON_GET(GL_STENCIL_BACK_PASS_DEPTH_PASS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilPass)
END_CASE

ON_GET(GL_STENCIL_BACK_REF):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilRef)
END_CASE

ON_GET(GL_STENCIL_BACK_VALUE_MASK):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilMask)
END_CASE

ON_GET(GL_STENCIL_BACK_WRITEMASK):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilWriteMask)
END_CASE

ON_GET(GL_STENCIL_BITS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, getFBStencilBits(ctx))
END_CASE

ON_GET(GL_STENCIL_CLEAR_VALUE):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->clearStencil)
END_CASE

ON_GET(GL_STENCIL_FAIL):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilFail)
END_CASE

ON_GET(GL_STENCIL_FUNC):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilFunc)
END_CASE

ON_GET(GL_STENCIL_PASS_DEPTH_FAIL):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilDepthFail)
END_CASE

ON_GET(GL_STENCIL_PASS_DEPTH_PASS):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilPass)
END_CASE

ON_GET(GL_STENCIL_REF):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilRef)
END_CASE

ON_GET(GL_STENCIL_TEST):
    SET_TYPE(BOOL)
    SET_NUM_PARAMS(1)
    SET_BOOL_PARAM(1, ctx->stencilTest ? GL_TRUE : GL_FALSE)
END_CASE

ON_GET(GL_STENCIL_VALUE_MASK):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilMask)
END_CASE

ON_GET(GL_STENCIL_WRITEMASK):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->stencilWriteMask)
END_CASE

// ON_GET(GL_SUBPIXEL_BITS):
    // TODO: params returns one value, an estimate of the number of bits of subpixel resolution that are used
    // to position rasterized geometry in window coordinates. The value must be at least 4.
// END_CASE

ON_GET(GL_TEXTURE_BINDING_2D): {
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)

    GLuint name = ctx->textureUnits[ctx->activeTextureUnit];
    const TextureInfo* tex = (TextureInfo*)name;

    if (tex && tex->target != GL_TEXTURE_2D)
        name = 0;

    SET_INT_PARAM(0, name)
    END_CASE
}

ON_GET(GL_TEXTURE_BINDING_CUBE_MAP): {
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)

    GLuint name = ctx->textureUnits[ctx->activeTextureUnit];
    const TextureInfo* tex = (TextureInfo*)name;

    if (tex && tex->target != GL_TEXTURE_CUBE_MAP)
        name = 0;

    SET_INT_PARAM(0, name)
}
END_CASE

ON_GET(GL_UNPACK_ALIGNMENT):
    SET_TYPE(INT)
    SET_NUM_PARAMS(1)
    SET_INT_PARAM(0, ctx->unpackAlignment)
END_CASE

ON_GET(GL_VIEWPORT):
    SET_TYPE(INT)
    SET_NUM_PARAMS(4)
    
    SET_INT_PARAM(0, ctx->viewportX)
    SET_INT_PARAM(1, ctx->viewportY)
    SET_INT_PARAM(2, ctx->viewportW)
    SET_INT_PARAM(3, ctx->viewportH)
END_CASE