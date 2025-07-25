// Adapted from https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/gpu/enums.h

/**
 * Copyright (c) 2014-2025 libctru contributors
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef _GLASS_PLATFORM_GPUDEFS_H
#define _GLASS_PLATFORM_GPUDEFS_H

#define ATTRIBFMT(i, n, f) (((((n)-1)<<2)|((f)&3))<<((i)*4))

typedef enum {
    RBFMT_RGBA8 = 0x00,
    RBFMT_RGB8 = 0x01,
    RBFMT_RGBA5551 = 0x02,
    RBFMT_RGB565 = 0x03,
    RBFMT_RGBA4 = 0x04,
    RBFMT_DEPTH16 = 0x00,
    RBFMT_DEPTH24 = 0x02,
    RBFMT_DEPTH24_STENCIL8 = 0x03,
} GPURenderbufferFormat;

typedef enum {
    ATTRIBTYPE_BYTE = 0x00,
    ATTRIBTYPE_UNSIGNED_BYTE = 0x01,
    ATTRIBTYPE_SHORT = 0x02,
    ATTRIBTYPE_FLOAT = 0x03,
} GPUAttribType;

typedef enum {
    COMBINERSRC_PRIMARY_COLOR = 0x00,
    COMBINERSRC_FRAGMENT_PRIMARY_COLOR = 0x01,
    COMBINERSRC_FRAGMENT_SECONDARY_COLOR = 0x02,
    COMBINERSRC_TEXTURE0 = 0x03,
    COMBINERSRC_TEXTURE1 = 0x04,
    COMBINERSRC_TEXTURE2 = 0x05,
    COMBINERSRC_TEXTURE3 = 0x06,
    COMBINERSRC_PREVIOUS_BUFFER = 0x0D,
    COMBINERSRC_CONSTANT = 0x0E,
    COMBINERSRC_PREVIOUS = 0x0F,
} GPUCombinerSource;

typedef enum {
    COMBINEROPRGB_SRC_COLOR = 0x00,
    COMBINEROPRGB_ONE_MINUS_SRC_COLOR = 0x01,
    COMBINEROPRGB_SRC_ALPHA = 0x02,
    COMBINEROPRGB_ONE_MINUS_SRC_ALPHA = 0x03,
    COMBINEROPRGB_SRC_R = 0x04,
    COMBINEROPRGB_ONE_MINUS_SRC_R = 0x05,
    COMBINEROPRGB_SRC_G = 0x08,
    COMBINEROPRGB_ONE_MINUS_SRC_G = 0x09,
    COMBINEROPRGB_SRC_B = 0x0C,
    COMBINEROPRGB_ONE_MINUS_SRC_B = 0x0D,
} GPUCombinerOpRGB;

typedef enum {
    COMBINEROPALPHA_SRC_ALPHA = 0x00,
    COMBINEROPALPHA_ONE_MINUS_SRC_ALPHA = 0x01,
    COMBINEROPALPHA_SRC_R = 0x02,
    COMBINEROPALPHA_ONE_MINUS_SRC_R = 0x03,
    COMBINEROPALPHA_SRC_G = 0x04,
    COMBINEROPALPHA_ONE_MINUS_SRC_G = 0x05,
    COMBINEROPALPHA_SRC_B = 0x06,
    COMBINEROPALPHA_ONE_MINUS_SRC_B = 0x07,
} GPUCombinerOpAlpha;

typedef enum {
    COMBINERFUNC_REPLACE = 0x00,
    COMBINERFUNC_MODULATE = 0x01,
    COMBINERFUNC_ADD = 0x02,
    COMBINERFUNC_ADD_SIGNED = 0x03,
    COMBINERFUNC_INTERPOLATE = 0x04,
    COMBINERFUNC_SUBTRACT = 0x05,
    COMBINERFUNC_DOT3_RGB = 0x06,
    COMBINERFUNC_DOT3_RGBA = 0x07,
    COMBINERFUNC_MULTIPLY_ADD = 0x08,
    COMBINERFUNC_ADD_MULTIPLY = 0x09,
} GPUCombinerFunc;

typedef enum {
    COMBINERSCALE_1 = 0x0,
    COMBINERSCALE_2 = 0x1,
    COMBINERSCALE_4 = 0x2,
} GPUCombinerScale;

typedef enum {
    FRAGOPMODE_DEFAULT = 0x00,
    FRAGOPMODE_GAS = 0x01,
    FRAGOPMODE_SHADOW = 0x03,
} GPUFragOpMode;

typedef enum {
    TESTFUNC_NEVER = 0x00,
    TESTFUNC_ALWAYS = 0x01,
    TESTFUNC_EQUAL = 0x02,
    TESTFUNC_NOTEQUAL = 0x03,
    TESTFUNC_LESS = 0x04,
    TESTFUNC_LEQUAL = 0x05,
    TESTFUNC_GREATER = 0x06,
    TESTFUNC_GEQUAL = 0x07,
} GPUTestFunc;

typedef enum {
    STENCILOP_KEEP = 0x00,
    STENCILOP_ZERO = 0x01,
    STENCILOP_REPLACE = 0x02,
    STENCILOP_INCR = 0x03,
    STENCILOP_DECR = 0x04,
    STENCILOP_INVERT = 0x05,
    STENCILOP_INCR_WRAP = 0x06,
    STENCILOP_DECR_WRAP = 0x07,
} GPUStencilOp;

typedef enum {
    CULLMODE_NONE = 0x00,
    CULLMODE_FRONT_CCW = 0x01,
    CULLMODE_BACK_CCW  = 0x02,
} GPUCullMode;

typedef enum {
    BLENDEQ_ADD = 0x00,
    BLENDEQ_SUBTRACT = 0x01,
    BLENDEQ_REVERSE_SUBTRACT = 0x02,
    BLENDEQ_MIN = 0x03,
    BLENDEQ_MAX = 0x04,
} GPUBlendEq;

typedef enum {
    BLENDFACTOR_ZERO = 0x00,
    BLENDFACTOR_ONE = 0x01,
    BLENDFACTOR_SRC_COLOR = 0x02,
    BLENDFACTOR_ONE_MINUS_SRC_COLOR = 0x03,
    BLENDFACTOR_DST_COLOR = 0x04,
    BLENDFACTOR_ONE_MINUS_DST_COLOR = 0x05,
    BLENDFACTOR_SRC_ALPHA = 0x06,
    BLENDFACTOR_ONE_MINUS_SRC_ALPHA = 0x07,
    BLENDFACTOR_DST_ALPHA = 0x08,
    BLENDFACTOR_ONE_MINUS_DST_ALPHA = 0x09,
    BLENDFACTOR_CONSTANT_COLOR = 0x0A,
    BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR = 0x0B,
    BLENDFACTOR_CONSTANT_ALPHA = 0x0C,
    BLENDFACTOR_ONE_MINUS_CONSTANT_ALPHA = 0x0D,
    BLENDFACTOR_SRC_ALPHA_SATURATE = 0x0E,
} GPUBlendFactor;

typedef enum {
    LOGICOP_CLEAR = 0x00,
    LOGICOP_AND = 0x01,
    LOGICOP_AND_REVERSE = 0x02,
    LOGICOP_COPY = 0x03,
    LOGICOP_SET = 0x04,
    LOGICOP_COPY_INVERTED = 0x05,
    LOGICOP_NOOP = 0x06,
    LOGICOP_INVERT = 0x07,
    LOGICOP_NAND = 0x08,
    LOGICOP_OR = 0x09,
    LOGICOP_NOR = 0x0A,
    LOGICOP_XOR = 0x0B,
    LOGICOP_EQUIV = 0x0C,
    LOGICOP_AND_INVERTED = 0x0D,
    LOGICOP_OR_REVERSE = 0x0E,
    LOGICOP_OR_INVERTED = 0x0F,
} GPULogicOp;

typedef enum {
    PRIMITIVE_TRIANGLES = 0x00,
    PRIMITIVE_TRIANGLE_STRIP = 0x01,
    PRIMITIVE_TRIANGLE_FAN = 0x02,
    PRIMITIVE_GEOMETRY = 0x03,
} GPUPrimitive;

typedef enum {
    TEXFILTER_NEAREST = 0x00,
    TEXFILTER_LINEAR = 0x01,
} GPUTexFilter;

typedef enum {
    TEXWRAP_CLAMP_TO_EDGE = 0x00,
    TEXWRAP_CLAMP_TO_BORDER = 0x01,
    TEXWRAP_REPEAT = 0x02,
    TEXWRAP_MIRRORED_REPEAT = 0x03,
} GPUTexWrap;

typedef enum {
    TEXMODE_2D = 0x00,
    TEXMODE_CUBEMAP = 0x01,
    TEXMODE_SHADOW_2D = 0x02,
    TEXMODE_PROJECTION = 0x03,
    TEXMODE_SHADOW_CUBE = 0x04,
    TEXMODE_DISABLED = 0x05,
} GPUTexMode;

typedef enum {
    SCISSORMODE_DISABLE = 0x00,
    SCISSORMODE_INVERTED = 0x01,
    SCISSORMODE_NORMAL = 0x03,
} GPUScissorMode;

typedef enum {
    EARLYDEPTHFUNC_GEQUAL = 0x00,
    EARLYDEPTHFUNC_GREATER = 0x01,
    EARLYDEPTHFUNC_LEQUAL = 0x02,
    EARLYDEPTHFUNC_LESS = 0x03,
} GPUEarlyDepthFunc;

typedef enum {
    TEXFORMAT_RGBA8 = 0x00,
    TEXFORMAT_RGB8 = 0x01,
    TEXFORMAT_RGB5A1 = 0x02,
    TEXFORMAT_RGB565 = 0x03,
    TEXFORMAT_RGBA4 = 0x04,
    TEXFORMAT_LA8 = 0x05,
    TEXFORMAT_HILO8 = 0x06,
    TEXFORMAT_L8 = 0x07,
    TEXFORMAT_A8 = 0x08,
    TEXFORMAT_LA4 = 0x09,
    TEXFORMAT_L4 = 0x0A,
    TEXFORMAT_A4 = 0x0B,
    TEXFORMAT_ETC1 = 0x0C,
    TEXFORMAT_ETC1A4 = 0x0D,
} GPUTexFormat;

typedef enum {
    GEOSHADERMODE_POINT = 0x00,
    GEOSHADERMODE_VARIABLE = 0x01,
    GEOSHADERMODE_FIXED = 0x02,
} GPUGeoShaderMode;

typedef enum {
    OUTPUTREGTYPE_POSITION = 0x00,
    OUTPUTREGTYPE_NORMALQUAT = 0x01,
    OUTPUTREGTYPE_COLOR = 0x02,
    OUTPUTREGTYPE_TEXCOORD0 = 0x03,
    OUTPUTREGTYPE_TEXCOORD0W = 0x04,
    OUTPUTREGTYPE_TEXCOORD1 = 0x05,
    OUTPUTREGTYPE_TEXCOORD2 = 0x06,
    OUTPUTREGTYPE_VIEW = 0x08,
    OUTPUTREGTYPE_DUMMY = 0x09,
} GPUOutputRegType;

#endif /* _GLASS_PLATFORM_GPUDEFS_H */