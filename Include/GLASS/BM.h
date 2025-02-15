#ifndef _GLASS_BM_H
#define _GLASS_BM_H

//#include <mem_map.h>
//#include <drivers/gfx.h>

//
#include <stdbool.h>
#include <stddef.h>
//

typedef enum
{
	GFX_LCD_TOP = 0u,
	GFX_LCD_BOT = 1u
} GfxLcd;

typedef enum
{
	GFX_SIDE_LEFT  = 0u,
	GFX_SIDE_RIGHT = 1u
} GfxSide;

typedef unsigned int u32;
typedef int s32;
typedef unsigned char u8;
typedef unsigned short u16;

/* COMPAT */

typedef enum vramAllocPos
{
	VRAM_ALLOC_A   = 0,
	VRAM_ALLOC_B   = 1,
	VRAM_ALLOC_ANY = VRAM_ALLOC_A | VRAM_ALLOC_B,
} vramAllocPos;

#endif /* _GLASS_BM_H */