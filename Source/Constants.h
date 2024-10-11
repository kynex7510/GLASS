#ifndef _GLASS_CONSTANTS_H
#define _GLASS_CONSTANTS_H

#define DECL_FLAG(id) (u32)(1u << (id))

#define GLASS_INVALID_OBJECT 0

#define GLASS_NUM_ATTRIB_REGS 16
#define GLASS_MAX_ENABLED_ATTRIBS 12
#define GLASS_NUM_ATTRIB_BUFFERS 12
#define GLASS_NUM_BOOL_UNIFORMS 16
#define GLASS_NUM_INT_UNIFORMS 4
#define GLASS_NUM_FLOAT_UNIFORMS 96
#define GLASS_NUM_COMBINER_STAGES 6
#define GLASS_NUM_TEXTURE_UNITS 3
#define GLASS_MAX_FB_WIDTH 1024
#define GLASS_MAX_FB_HEIGHT 1024

#define GLASS_UNI_BOOL 0x00
#define GLASS_UNI_INT 0x01
#define GLASS_UNI_FLOAT 0x02

#define GLASS_BUFFER_TYPE 0x01
#define GLASS_TEXTURE_TYPE 0x02
#define GLASS_PROGRAM_TYPE 0x03
#define GLASS_SHADER_TYPE 0x04
#define GLASS_FRAMEBUFFER_TYPE 0x05
#define GLASS_RENDERBUFFER_TYPE 0x06

#define BUFFER_FLAG_BOUND DECL_FLAG(0)

#define TEXTURE_FLAG_BOUND DECL_FLAG(0)

#define RENDERBUFFER_FLAG_BOUND DECL_FLAG(0)

#define FRAMEBUFFER_FLAG_BOUND DECL_FLAG(0)

#define SHADER_FLAG_DELETE DECL_FLAG(0)
#define SHADER_FLAG_GEOMETRY DECL_FLAG(1)
#define SHADER_FLAG_MERGE_OUTMAPS DECL_FLAG(2)
#define SHADER_FLAG_USE_TEXCOORDS DECL_FLAG(3)

#define PROGRAM_FLAG_DELETE DECL_FLAG(0)
#define PROGRAM_FLAG_LINK_FAILED DECL_FLAG(1)
#define PROGRAM_FLAG_UPDATE_VERTEX DECL_FLAG(2)
#define PROGRAM_FLAG_UPDATE_GEOMETRY DECL_FLAG(3)

#define ATTRIB_FLAG_ENABLED DECL_FLAG(0)
#define ATTRIB_FLAG_FIXED DECL_FLAG(1)

#define CONTEXT_FLAG_FRAMEBUFFER DECL_FLAG(0)
#define CONTEXT_FLAG_DRAW DECL_FLAG(1)
#define CONTEXT_FLAG_VIEWPORT DECL_FLAG(2)
#define CONTEXT_FLAG_SCISSOR DECL_FLAG(3)
#define CONTEXT_FLAG_ATTRIBS DECL_FLAG(4)
#define CONTEXT_FLAG_PROGRAM DECL_FLAG(5)
#define CONTEXT_FLAG_COMBINERS DECL_FLAG(6)
#define CONTEXT_FLAG_FRAGMENT DECL_FLAG(7)
#define CONTEXT_FLAG_DEPTHMAP DECL_FLAG(8)
#define CONTEXT_FLAG_COLOR_DEPTH DECL_FLAG(9)
#define CONTEXT_FLAG_EARLY_DEPTH DECL_FLAG(10)
#define CONTEXT_FLAG_EARLY_DEPTH_CLEAR DECL_FLAG(11)
#define CONTEXT_FLAG_STENCIL DECL_FLAG(12)
#define CONTEXT_FLAG_CULL_FACE DECL_FLAG(13)
#define CONTEXT_FLAG_ALPHA DECL_FLAG(14)
#define CONTEXT_FLAG_BLEND DECL_FLAG(15)
#define CONTEXT_FLAG_TEXTURE DECL_FLAG(16)
#define CONTEXT_FLAG_ALL (~(0u))

#endif /* _GLASS_CONSTANTS_H */