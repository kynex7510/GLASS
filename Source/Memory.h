#ifndef _GLASS_MEMORY_H
#define _GLASS_MEMORY_H

#include "Types.h"

#include <string.h>

void* GLASS_virtualAlloc(size_t size);
void GLASS_virtualFree(void* p);
void* GLASS_linearAlloc(size_t size);
void GLASS_linearFree(void* p);
void* GLASS_vramAlloc(size_t size, vramAllocPos pos);
void GLASS_vramFree(void* p);

#endif /* _GLASS_MEM_H */