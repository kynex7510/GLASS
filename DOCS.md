# Docs

## Caveats

- Fragment pipeline is not programmable;
- No default framebuffer, nor default shader program, is provided.

## Attributes

The GPU has 16 input registers, but at most 12 can be used at the same time: `glEnableVertexAttribArray` will fail with `GL_INVALID_OPERATION` if all 12 are used. To use a different register first call `glDisableVertexAttribArray` to disable one of the used registers.

When calling `glVertexAttribPointer`, the `stride` argument must match the size of the attribute buffer. Moreover, if a raw buffer is passed as the `pointer` argument, said buffer must be allocated on linear heap, and must be flushed before being used.

## Differences from the standard

- `glBind*` functions will not create buffers when passing arbitrary buffer values, instead `glGen*` must be used for buffer creation;
- Unlike the OpenGL specs which limit each shader type to 1 entry, `glShaderBinary` supports as much shader entries as are contained in the shader binary, and are loaded in the order they've been linked;
- `glPolygonOffset`: the `factor` argument has no effect;
- `glVertexAttribPointer`: the `normalized` argument must be set to `GL_FALSE`;
- `glDrawArrays`: the `mode` argument must be one of `GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_GEOMETRY_PRIMITIVE_PICA`;
- If the number of components of the uniform variable as defined in the shader does not match the size specified in the name of the command used to load its value, and the uniform variable is not of type `bool`, no error will be generated; the other components of the specified uniform variable will remain unchanged.

## Debugging

If GLASS doesn't work as intended, or is responsible for crashing applications, you can compile it in debug mode. Assertions will be enabled, and informations will be logged either under `sdmc:/GLASS.log`, or if GLASS is running under an emulator (eg. citra), they will be logged on the emulator console.

Note that GLASS only does error checking to a bare minimum (eg. anything specified by the OpenGL ES standard), for space and computation efficiency. It will not check for buffers to be valid, nor for certain parameters to be correct (thought most assertions catch these). Users of the library must take care when writing applications and make sure to follow the OpenGL ES documentation, or this documentation when the implementation deviates.

## Memory allocation

Internally, GLASS has to allocate various kinds of memory buffers to work. It's possible to customize allocation by redefining any of the following functions:

```c
void* GLASS_virtualAlloc(size_t size);
void GLASS_virtualFree(void* p);
size_t GLASS_virtualSize(void* p);

void* GLASS_linearAlloc(size_t size);
void GLASS_linearFree(void* p);
size_t GLASS_linearSize(void* p);

void* GLASS_vramAlloc(size_t size, vramAllocPos pos);
void GLASS_vramFree(void* p);
size_t GLASS_vramSize(void* p);
```

- `*Alloc` allocates `size` bytes;
- `*Free` frees the memory pointed by `p`;
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