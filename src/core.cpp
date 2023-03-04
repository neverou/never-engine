#include "core.h"


#ifdef PLATFORM_MACOS
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include "std.h"
#include "logger.h"

#include "game.h"

void* Alloc(size_t size) {  
	// printf("Allocated %d bytes!\n", cast(s32, size));

    if (size == 0) {
        return NULL;
    }
    else {
        void* ptr = malloc(size);
        
#if defined(BUILD_DEBUG)
        if (!ptr) {
            LogWarn("Allocation failed!"); // Allocation failed!
        }
#endif
        
        return ptr;
    }
}

void* Realloc(void* memory, size_t newSize) {
    //    printf("Reallocated %d bytes!\n", cast(s32, newSize));
    if (memory) {
        return realloc(memory, newSize);
    }
    else {
        return malloc(newSize);
    }
}

void Free(void* memory) {
    // printf("Freed!\n");
    if (memory) {
        free(memory);
    }
}
