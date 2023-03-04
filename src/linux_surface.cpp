#if defined(PLATFORM_LINUX)
#include "linux_surface.h"

#include "logger.h"
#include "std.h"

#define SDL_MAIN_HANDLED
#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>
#include "sdl_utils.h"


void LinuxSurface::Update()
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
            case SDL_QUIT: {
				Event event {};
				event.type = Event_Quit;
				eventBus->PushEvent(&event);
                break;
            }
			case SDL_KEYDOWN: {
				Event event {};
				event.type = Event_Keyboard;
				event.key.type = KEY_EVENT_DOWN;
                event.key.isRepeat = srcEvent.key.repeat != 0;
				event.key.key = GetKeycodeFromSdlKeycode(srcEvent.key.keysym.sym);
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_KEYUP: {
				Event event {};
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
            case SDL_MOUSEBUTTONDOWN: {
                Event event {};
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

            case SDL_MOUSEMOTION: {
                Event event {};
				
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

			case SDL_CONTROLLERBUTTONDOWN: {
				Event event {};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_BUTTON_DOWN;
				event.gamepad.button.id = srcEvent.cbutton.button;
                
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERBUTTONUP: {
				Event event {};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_BUTTON_UP;
				event.gamepad.button.id = srcEvent.cbutton.button;
                Log(SDL_GameControllerGetStringForButton((SDL_GameControllerButton)srcEvent.cbutton.button));
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERAXISMOTION: {
				Event event {};
				event.type = Event_Gamepad;
				event.gamepad.type = CONTROLLER_AXIS_MOTION;
				event.gamepad.axis.id = srcEvent.caxis.axis;
				event.gamepad.axis.value = srcEvent.caxis.value;
                
				eventBus->PushEvent(&event);
				break;
			}
			case SDL_CONTROLLERDEVICEADDED: {
                
				SDL_GameController* controller = SDL_GameControllerOpen(srcEvent.cdevice.which);

				Log("[sys] Controller device added (%s)", SDL_GameControllerName(controller));
                
				ArrayAdd(&gameControllerKeys, srcEvent.cdevice.which);
				ArrayAdd(&gameControllers, controller);
                
				// ~Todo event bus
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED: {
				Size index = ArrayFind(&gameControllerKeys, srcEvent.cdevice.which);
				Assert(index != -1);

				auto controller = gameControllers[index];
                
				Log("[sys] Controller device removed (%s)", SDL_GameControllerName(controller));
                
                
				SDL_GameControllerClose(controller);
                
				// ~Todo push to event bus

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

        }
	}
}

NativeSurfaceInfo LinuxSurface::GetNativeInfo()
{
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    
    NativeSurfaceInfo nativeInfo;
	nativeInfo.flags = NATIVE_SURFACE_IS_SDL_BACKEND;	
	nativeInfo.sdlWindow = window;
	
	// nativeInfo.window = wmInfo.info.x11.window;
	// nativeInfo.display = wmInfo.info.x11.display;
    return nativeInfo;
}




LinuxSurface* SpawnLinuxSurface(const SurfaceSpawnInfo* spawnInfo, Allocator* allocator) {
    LinuxSurface* surface = new (AceAlloc(sizeof(LinuxSurface), allocator)) LinuxSurface;
    surface->allocator = allocator;
    
    surface->eventBus = spawnInfo->eventBus;
    surface->rendererType = spawnInfo->rendererType;
    
    u32 flags = 0;
    
    switch (surface->rendererType) {
        case RENDERER_OPENGL: {
            flags |= SDL_WINDOW_OPENGL;
            break;
        }
        case RENDERER_VULKAN: {
            flags |= SDL_WINDOW_VULKAN;
            break;
        }
    }
    
    
    
	surface->gameControllers = MakeDynArray<SDL_GameController*>();
	surface->gameControllerKeys = MakeDynArray<s32>();
    
    
	// ~Temp
	// flags |= SDL_WINDOW_RESIZABLE;
    
    
    SDL_Init(SDL_INIT_EVERYTHING);
    
    // if we're using opengl make sure to configure the version and everything
    if (flags & SDL_WINDOW_OPENGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }
	surface->window = SDL_CreateWindow(spawnInfo->title.data, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, spawnInfo->width, spawnInfo->height, flags);
    
	if (!surface->window) {
        LogWarn(SDL_GetError());
		return NULL;
	}

	return surface;
}

void DeleteLinuxSurface(LinuxSurface* surface) {
    SDL_DestroyWindow(surface->window);
    SDL_Quit();
    
    AceFree(surface, surface->allocator);
    
    FreeDynArray(&surface->gameControllers);
    FreeDynArray(&surface->gameControllerKeys);
}

#endif
