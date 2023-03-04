#pragma once

#include "core.h"

struct Allocator {
    void* data;
    
    void* (*alloc)(void* data, Size size);
    void* (*realloc)(void* data, void* memory, Size oldSize, Size newSize);
    void (*free)(void* data, void* memory);
};



struct MemoryArena {
    void* base;
    Size used;
    Size size;
};

MemoryArena AllocArena(Size size);
void FreeArena(MemoryArena* arena);

void* PushAlloc(Size size, MemoryArena* arena);
void* StackRealloc(void* memory, Size sizeOld, Size sizeNew, MemoryArena* arena);

//

Allocator MakeMemoryArenaAllocator(MemoryArena* arena);

void* AceAlloc(Size size, Allocator* allo);
// ~Todo: maybe dont use oldSize somehow??
void* AceRealloc(void* mem, Size oldSize, Size newSize, Allocator* allo);
void AceFree(void* mem, Allocator* allo);