#include "memory.h"
#include "std.h"

MemoryArena AllocArena(Size size) {
    MemoryArena arena;
    arena.base = Alloc(size);
    arena.used = 0;
    arena.size = size;
    return arena;
}

void FreeArena(MemoryArena* arena) {
    Free(arena->base);
    arena->base = NULL;
    arena->used = 0;
    arena->size = 0;
}



void* PushAlloc(Size size, MemoryArena* arena) {
    if (arena->used + size >= arena->size) {
		return NULL;
	}
	
	void* memory = ((u8*)arena->base) + arena->used;
    arena->used += size;
	
    return memory;
}

void* StackRealloc(void* memory, Size sizeOld, Size sizeNew, MemoryArena* arena) {
    // in-place realloc
    if (!memory || sizeOld == 0) {
        return PushAlloc(sizeNew, arena);
    }
    
    if (memory == (cast(u8*, arena->base) + arena->used - sizeOld)) {
        arena->used += sizeNew - sizeOld;
        return memory;
    }
    else {
        void* newMem = PushAlloc(sizeNew, arena);
        Memcpy(newMem, memory, (sizeNew < sizeOld) ? sizeNew : sizeOld);
        return newMem;
    }
}

//

Allocator MakeMemoryArenaAllocator(MemoryArena* arena) {
    Allocator allocator;
    allocator.data = arena;
    allocator.alloc = [](void* data, Size size) -> void* {
        return PushAlloc(size, (MemoryArena*)data);
    };
    
    allocator.realloc = [](void* data, void* memory, Size sizeOld, Size sizeNew) -> void* {
        return StackRealloc(memory, sizeOld, sizeNew, (MemoryArena*)data);
    };
    allocator.free = NULL;
    return allocator;
}


void* AceAlloc(Size size, Allocator* allo) {
    if (allo) {
        Assert(allo->alloc);
        return allo->alloc(allo->data, size);
    }
    else
        return Alloc(size);
}

void* AceRealloc(void* mem, Size oldSize, Size newSize, Allocator* allo) {
    if (allo) {
        Assert(allo->realloc);
        return allo->realloc(allo->data, mem, oldSize, newSize);
    }
    else
        return Realloc(mem, newSize);
}

void AceFree(void* mem, Allocator* allo) {
    if (allo) {
        // the allocator doesnt need to be able to free necessarily, for example if its an arena
        if (allo->free)
            allo->free(allo->data, mem);
    }
    else
        Free(mem);
}