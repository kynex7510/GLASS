# Docs

## Differences from the standard

- Fragment pipeline is not programmable.
- No default objects are provided (framebuffer, textures, shader program).
- `glBind*` functions will not create buffers when passing arbitrary buffer values, instead `glGen*` must be used for buffer creation.
- Unlike the OpenGL specs which limit each shader type to 1 entry, `glShaderBinary` supports as much shader entries as are contained in the shader binary, and are loaded in the order they've been linked.
- `glPolygonOffset`: the `factor` argument has no effect.
- `glVertexAttribPointer`: the `normalized` argument must be set to `GL_FALSE`.
- `glDrawArrays`: the `mode` argument must be one of `GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_GEOMETRY_PRIMITIVE_PICA`.
- If the number of components of the uniform variable as defined in the shader does not match the size specified in the name of the command used to load its value, and the uniform variable is not of type `bool`, no error will be generated; the other components of the specified uniform variable will remain unchanged.
- `glClear`: scissor test and buffer writemasks are ignored.
- `glTexParameter`: `GL_TEXTURE_MIN_LOD`, `GL_TEXTURE_MAX_LOD`, `GL_TEXTURE_LOD_BIAS` are all valid parameters.
- `glTexParameter`: `GL_CLAMP_TO_BORDER` is a valid wrapping value.

## Attributes

The GPU has 16 input registers, but at most 12 can be used at the same time: `glEnableVertexAttribArray` will fail with `GL_INVALID_OPERATION` if all 12 are used. To use a different register first call `glDisableVertexAttribArray` to disable one of the used registers.

When calling `glVertexAttribPointer`, the `stride` argument must match the size of the attribute buffer. Moreover, if a raw buffer is passed as the `pointer` argument, said buffer must be allocated on linear heap, and must be flushed before being used.

## Textures

3 texture units are available. Only `GL_TEXTURE0` can load cube maps, and only one target at time can be used.

When specifying LOD values, they will be clamped in the range `[0, 15]`. When specifying a LOD bias, it will be clamped in the range `[-15.255, 15.255]`.

`GL_NEAREST` and `GL_NEAREST_MIPMAP_NEAREST` are the same, the GPU decides to use one or the other based on the value of `GL_TEXTURE_MIN_LOD` (ie. whether the filter is active). Same goes for `GL_LINEAR` and `GL_LINEAR_MIPMAP_NEAREST`.

## Memory allocation

Internally, GLASS has to allocate various kinds of memory buffers to work. It's possible to customize allocation by redefining any of the following functions:

```c
void* glassVirtualAlloc(size_t size);
void glassVirtualFree(void* p);
size_t glassVirtualSize(void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(void* p);

void* glassVRAMAlloc(size_t size, vramAllocPos pos);
void glassVRAMFree(void* p);
size_t glassVRAMSize(void* p);
```

- `*Alloc` allocates `size` bytes.
- `*Free` frees the memory pointed by `p`.
- `*Size` returns the usuable size for the memory pointed by `p`.

By redefining any of the functions above, you will override default memory management routines. This is useful for tracking, controlling and customizing allocations.

If you need to rely on the default behaviour, you can declare and call any of the following functions:

```c
void* GLASS_virtualAllocDefault(size_t size);
void GLASS_virtualFreeDefault(void* p);
size_t GLASS_virtualSizeDefault(void* p);

void* GLASS_linearAllocDefault(size_t size);
void GLASS_linearFreeDefault(void* p);
size_t GLASS_linearSizeDefault(void* p);

void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos);
void GLASS_vramFreeDefault(void* p);
size_t GLASS_vramSizeDefault(void* p);
```

## Debugging

If GLASS doesn't work as intended, or is responsible for crashing applications, you can compile it in debug mode. Assertions will be enabled, and informations will be logged either under `sdmc:/GLASS.log`, or if GLASS is running under an emulator, they will be logged on the emulator console.

By default, GLASS in debug mode will abort whenever an OpenGL error is raised. This behaviour can be controlled with the `GLASS_NO_MERCY` macro.

Note that GLASS only does error checking to a bare minimum (eg. anything specified by the OpenGL ES standard), for space and computation efficiency. It will not check for buffers to be valid, nor for certain parameters to be correct (thought most assertions catch these). Users of the library must take care when writing applications and make sure to follow the OpenGL ES documentation, or this documentation when the implementation deviates.