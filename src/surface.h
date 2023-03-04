#pragma once



#include "core.h"
#include "std.h"

#include "event.h"

enum WindowMode 
{
    WINDOWED,
    FULLSCREEN_DESKTOP,
    FULLSCREEN,
};




#if defined(PLATFORM_WINDOWS)
#include <windows.h>
// #elif defined(PLATFORM_LINUX)
// #include <X11/Xlib.h>
#endif



//NOTE: maybe switch this around so instead of passing the SDL window we create the renderer platform layer in the surface as a function
enum NativeSurfaceInfoFlags {
    NATIVE_SURFACE_NONE = 0,
    NATIVE_SURFACE_IS_SDL_BACKEND = 1,
};

struct SDL_Window;

struct NativeSurfaceInfo {
    u64 flags;
	SDL_Window* sdlWindow;
	
    #if defined(PLATFORM_WINDOWS)
    HWND hwnd;
    // #elif defined(PLATFORM_LINUX)
    // Window window;
    // Display* display;
    #endif
};

#include "renderer.h"


struct Surface {
    // memory management stuff //
    Allocator* allocator;
    //

	EventBus* eventBus;
	RendererType rendererType;
	
    virtual void Update() = 0;
    virtual NativeSurfaceInfo GetNativeInfo() = 0;

    // TODO: add these 
    // virtual void SetSize(u32 width, u32 height) = 0;
    // virtual void SetWindowMode(WindowMode windowMode) = 0;
    // virtual void SetTitle(String title) = 0;
};

 

// NOTE: these might get ignored on certain platforms
// such as Xbox or Nintendo Switch, or mobile (ew), where
// the window (technically a surface) will be given to you 
// with prechosen settings
// - ... 2/5/2020
struct SurfaceSpawnInfo
{
    StringView title;
    u32 width;
    u32 height;

	RendererType rendererType;

	EventBus* eventBus;
};

Surface* SpawnSurface(const SurfaceSpawnInfo* spawnInfo, Allocator* allocator); 
void DeleteSurface(Surface* window);
