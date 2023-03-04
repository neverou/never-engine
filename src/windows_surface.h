#pragma once

#if defined(PLATFORM_WINDOWS)

#include "surface.h"


struct SDL_Window;
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;
struct WindowsSurface : public Surface
{
    SDL_Window* window;
    void Update() override;
    NativeSurfaceInfo GetNativeInfo() override;
    // Storing the connected GameControllers' Joystick ID
    DynArray<SDL_GameController*> gameControllers; // actual pointer to the controller obj
    DynArray<s32> gameControllerKeys; // joystick ID that corresponds to the controller obj
};

WindowsSurface* SpawnWindowsSurface(const SurfaceSpawnInfo* spawn_info, Allocator* allocator);
void DeleteWindowsSurface(WindowsSurface* surface);

#endif
