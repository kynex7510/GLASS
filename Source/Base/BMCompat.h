#ifndef _GLASS_BASE_BMCOMPAT_H
#define _GLASS_BASE_BMCOMPAT_H

// Macros and types for ctru <-> n3ds compatibility.
// https://github.com/devkitPro/libctru

#if defined(GLASS_BAREMETAL)

typedef enum {
	GSH_POINT = 0,
	GSH_VARIABLE_PRIM = 1,
	GSH_FIXED_PRIM = 2,
} DVLE_geoShaderMode;

typedef enum {
	GPU_SCISSOR_DISABLE = 0,
	GPU_SCISSOR_INVERT = 1,
	GPU_SCISSOR_NORMAL = 3,
} GPU_SCISSORMODE;

typedef enum {
	GPU_EARLYDEPTH_GEQUAL = 0,
	GPU_EARLYDEPTH_GREATER = 1,
	GPU_EARLYDEPTH_LEQUAL = 2,
	GPU_EARLYDEPTH_LESS = 3,
} GPU_EARLYDEPTHFUNC;

typedef enum {
	GPU_RGBA8 = 0x0,
	GPU_RGB8 = 0x1,
	GPU_RGBA5551 = 0x2,
	GPU_RGB565 = 0x3,
	GPU_RGBA4 = 0x4,
	GPU_LA8 = 0x5,
	GPU_HILO8 = 0x6,
	GPU_L8 = 0x7,
	GPU_A8 = 0x8,
	GPU_LA4 = 0x9,
	GPU_L4 = 0xA,
	GPU_A4 = 0xB,
	GPU_ETC1 = 0xC,
	GPU_ETC1A4 = 0xD,
} GPU_TEXCOLOR;

#endif // GLASS_BAREMETAL

#endif /* _GLASS_BASE_BMCOMPAT_H */