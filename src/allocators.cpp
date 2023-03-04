#include "allocators.h"

intern Allocators allocators; 

Allocators* GetAllocators() {
    Allocators* ptrToAllocators = &allocators;

	local_persist bool hasInit = false;

	if (!hasInit) 
	{
		hasInit = true;

		ptrToAllocators->frameArena = AllocArena(Gigabytes(1));
		ptrToAllocators->frameAllocator = MakeMemoryArenaAllocator(&ptrToAllocators->frameArena);

		ptrToAllocators->engineArena = AllocArena(Megabytes(8));
		ptrToAllocators->engineAllocator = MakeMemoryArenaAllocator(&ptrToAllocators->engineArena);
	}

    return ptrToAllocators;
}

void FreeAllocators()
{
	FreeArena(&allocators.frameArena);
	FreeArena(&allocators.engineArena);
}