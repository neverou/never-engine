#if defined(PLATFORM_WINDOWS)

#include "windows_surface.h"

#include "logger.h"
#include "array.h"

#define SDL_MAIN_HANDLED
#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>
#include "sdl_utils.h"


void WindowsSurface::Update()
{
	SDL_Event srcEvent;

	// event loop
	while (SDL_PollEvent(&srcEvent)) {
		// NOTE:
		// for now the event bus should always be set, but in the future we might change this? (level editor, other applications of the engine framework for tooling?)
		// but also we might require them to have an event but if we make all the other engine systems sparse.
		// i think we should just require the engine to not be sparse
		Assert(eventBus != NULL);

		switch (srcEvent.type) {
			case SDL_QUIT:
			{
				Event event{};
				event.type = Event_Quit;
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_KEYDOWN:
			{
				Event event{};
				event.type = Event_Keyboard;
				event.key.type = KEY_EVENT_DOWN;
				event.key.isRepeat = srcEvent.key.repeat != 0;
				event.key.key = GetKeycodeFromSdlKeycode(srcEvent.key.keysym.sym);
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_KEYUP:
			{
				Event event{};
				event.type = Event_Keyboard;
				event.key.type = KEY_EVENT_UP;
				event.key.isRepeat = false;
				event.key.key = GetKeycodeFromSdlKeycode(srcEvent.key.keysym.sym);
				eventBus->PushEvent(&event);
				break;
			}

			case SDL_MOUSEWHEEL:
			{
                Event event {};
                event.type = Event_Mouse;
				event.mouse.type = Mouse_Scroll;
				event.mouse.scrollDelta = srcEvent.wheel.y;
				eventBus->PushEvent(&event);	
				break;
			}

			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			{
				Event event{};
				event.type = Event_Mouse;

				if (srcEvent.type == SDL_MOUSEBUTTONDOWN) event.mouse.type = MOUSE_BUTTON_DOWN;
				else if (srcEvent.type == SDL_MOUSEBUTTONUP) event.mouse.type = MOUSE_BUTTON_UP;


				u8 button = 0;
				switch (srcEvent.button.button) {
					case SDL_BUTTON_LEFT:   button = MB_Left; break;
					case SDL_BUTTON_RIGHT:  button = MB_Right; break;
					case SDL_BUTTON_MIDDLE: button = MB_Middle; break;
					case SDL_BUTTON_X1:     button = MB_Mouse4; break;
					case SDL_BUTTON_X2:     button = MB_Mouse5; break;
				}

				event.mouse.buttonId = button;

				eventBus->PushEvent(&event);
				break;
			}

			case SDL_MOUSEMOTION:
			{
				Event event{};
				event.type = Event_Mouse;
				event.mouse.type = MOUSE_MOVE;
				event.mouse.moveDeltaX = srcEvent.motion.xrel;
				event.mouse.moveDeltaY = -srcEvent.motion.yrel;
				event.mouse.posX = srcEvent.motion.x;

				s32 ySize;
				SDL_GetWindowSize(window, NULL, &ySize);
				event.mouse.posY = ySize - 1 - srcEvent.motion.y;

				eventBus->PushEvent(&event);
				break;
			}

			case SDL_CONTROLLERBUTTONDOWN:
			{
				Event event{};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_BUTTON_DOWN;
				event.gamepad.button.id = srcEvent.cbutton.button;

				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERBUTTONUP:
			{
				Event event{};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_BUTTON_UP;
				event.gamepad.button.id = srcEvent.cbutton.button;
				Log(SDL_GameControllerGetStringForButton((SDL_GameControllerButton)srcEvent.cbutton.button));
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERAXISMOTION:
			{
				Event event{};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_AXIS_MOTION;
				event.gamepad.axis.id = srcEvent.caxis.axis;
				event.gamepad.axis.value = srcEvent.caxis.value;

				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERDEVICEADDED:
			{
				Log("Contorller device added");

				SDL_GameController* controller = SDL_GameControllerOpen(srcEvent.cdevice.which);

				ArrayAdd(&gameControllerKeys, srcEvent.cdevice.which);
				ArrayAdd(&gameControllers, controller);

				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED:
			{
				Log("Contorller device removed");

				Size index = ArrayFind(&gameControllerKeys, srcEvent.cdevice.which);

				Assert(index != -1);

				SDL_GameControllerClose(gameControllers[index]);

				ArrayRemoveAt(&gameControllerKeys, index);
				ArrayRemoveAt(&gameControllers, index);

				break;
			}
			case SDL_WINDOWEVENT:
			{
				switch (srcEvent.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					{
						Event event = {};
						event.type = Event_SurfaceResize;
						event.surfaceResize.width = srcEvent.window.data1;
						event.surfaceResize.height = srcEvent.window.data2;

						eventBus->PushEvent(&event);
						break;
					}
				}

				break;
			}

			// default: {
			//     // lol this is kinda unneccessary
			//     // LogWarn("unhandled SDL event!");
			//     break;
			// }
		}
	}
}

NativeSurfaceInfo WindowsSurface::GetNativeInfo()
{
    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(window, &wm_info);
    
    NativeSurfaceInfo native_info;
	native_info.flags = NATIVE_SURFACE_IS_SDL_BACKEND;
	native_info.sdlWindow = window;
	
    native_info.hwnd = wm_info.info.win.window;
    return native_info;
}


WindowsSurface* SpawnWindowsSurface(const SurfaceSpawnInfo* spawnInfo, Allocator* allocator)
{
    WindowsSurface* surface = new (AceAlloc(sizeof(WindowsSurface), allocator)) WindowsSurface;
    surface->allocator = allocator;
    
	surface->rendererType = spawnInfo->rendererType;
	surface->eventBus = spawnInfo->eventBus;
    
	surface->gameControllers = MakeDynArray<SDL_GameController*>();
	surface->gameControllerKeys = MakeDynArray<s32>();
	
	u32 flags = 0;
    
	// NOTE(...): there might be a better solution for this?
	// for now im just using no virtual functions tho,
	// cause maybe like, surface->spawn_rendererer() can recreate the window with different flags?
	// seems janky tho
	
	switch (surface->rendererType) {
        
#if defined(RENDERER_IMPL_OPENGL)
		case RENDERER_OPENGL: {
			flags |= SDL_WINDOW_OPENGL;
			break;
		}
#endif
        
#if defined(RENDERER_IMPL_VULKAN)
		case RENDERER_VULKAN: {
			flags |= SDL_WINDOW_VULKAN;
			break;
		}
#endif
        // Not needed for DirectX because we create the device context ourselves.
	}
	
    
    
    
    SDL_Init(SDL_INIT_EVERYTHING);
    
    // if we're using opengl make sure to configure the version and everything
    if (flags & SDL_WINDOW_OPENGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }
    
    surface->window = SDL_CreateWindow(spawnInfo->title.data, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, spawnInfo->width, spawnInfo->height, flags); // TEMP
	   
	if (!surface->window) {
        LogWarn(SDL_GetError());
		return NULL;
	}
    
    return surface;
}

void DeleteWindowsSurface(WindowsSurface* surface)
{
    SDL_DestroyWindow(surface->window);
    SDL_Quit();
    
    AceFree(surface, surface->allocator);
	FreeDynArray(&surface->gameControllers);
	FreeDynArray(&surface->gameControllerKeys);
}


#endif
