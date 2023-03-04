#pragma once

#if defined(PLATFORM_LINUX)
#include "surface.h"

struct SDL_Window;
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;
struct LinuxSurface : public Surface {
	SDL_Window* window;

    void Update() override;
	NativeSurfaceInfo GetNativeInfo() override;

    // Storing the connected GameControllers' Joystick ID
    DynArray<SDL_GameController*> gameControllers; // actual pointer to the controller obj
    DynArray<s32> gameControllerKeys; // joystick ID that corresponds to the controller obj
};

LinuxSurface* SpawnLinuxSurface(const SurfaceSpawnInfo* spawnInfo, Allocator* allocator);
void DeleteLinuxSurface(LinuxSurface* surface);

#endif
