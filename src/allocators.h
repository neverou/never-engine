#pragma once
#include "memory.h"

#define Frame_Arena (&GetAllocators()->frameAllocator)
#define Engine_Arena (&GetAllocators()->engineAllocator)

struct Allocators {
    MemoryArena frameArena;
    Allocator frameAllocator;
    
    MemoryArena engineArena;
    Allocator engineAllocator;
};

Allocators* GetAllocators();
void FreeAllocators();