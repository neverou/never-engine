#include "surface.h"

#if defined(PLATFORM_WINDOWS)
#include "windows_surface.h"
#elif defined(PLATFORM_LINUX)
#include "linux_surface.h"
#endif

#include "logger.h"

Surface* SpawnSurface(const SurfaceSpawnInfo* spawnInfo, Allocator* allocator) 
{
#if defined(PLATFORM_WINDOWS)
    
    return SpawnWindowsSurface(spawnInfo, allocator);    
    
#elif defined(PLATFORM_LINUX)
    
    return SpawnLinuxSurface(spawnInfo, allocator);
    
#endif
    
    LogError("Failed to create surface: Platform not supported!");
    Assert(false); // Not supported
    return NULL;
}

void DeleteSurface(Surface* surface)
{
#if defined(PLATFORM_WINDOWS)
    DeleteWindowsSurface((WindowsSurface*)surface);
#elif defined(PLATFORM_LINUX)
	DeleteLinuxSurface((LinuxSurface*)surface);
#else
    
    LogError("Failed to delete surface: Platform not supported!");
    Assert(false); // Not supported
#endif
}
