#include "game.h"

#include "std.h"
#include "surface.h"
#include "renderer.h"
#include "logger.h"
#include "resource.h"
#include "gizmos.h"
#include "allocators.h"
#include "entityutil.h"
#include "editor.h"
#include "audio.h"

#include "anim.cpp"
#include "entity.cpp"
#include "entityutil.cpp"
#include "allocators.cpp"
#include "array.cpp"
// We will eventually add this back but because MSVC is horrible it causes issues when we add the C++ standard library (currently used for threads) so thats not happening until we get rid of it...
// #include "audio.cpp"
#include "audio_linux.cpp"
#include "audio_windows.cpp"
#include "character_controller.cpp"
#include "combat_manager.cpp"
#include "core.cpp"
#include "d3d11_renderer.cpp"
#include "editor.cpp"
#include "enemy.cpp"
#include "error.cpp"
#include "event.cpp"
#include "gfx.cpp"
#include "game_manager.cpp"
#include "gizmos.cpp"
#include "gui.cpp"
#include "linux_surface.cpp"
#include "logger.cpp"
#include "material.cpp"
#include "maths.cpp"
#include "memory.cpp"
#include "mesh.cpp"
#include "monkey_easter_egg.cpp"
#include "opengl_renderer.cpp"
#include "physics.cpp"
#include "player.cpp"
#include "prop.cpp"
#include "renderer.cpp"
#include "resource.cpp"
#include "shaderparser.cpp"
#include "str.cpp"
#include "surface.cpp"
#include "sys_linux.cpp"
#include "sys_macos.cpp"
#include "sys_windows.cpp"
#include "telemetry.cpp"
#include "terrain.cpp"
#include "util.cpp"
#include "vulkan_renderer.cpp"
#include "windows_surface.cpp"
#include "world.cpp"
#include "zefir.cpp"
#include "hunter.cpp"
#include "stb/stb.cpp"
#include "glad/glad.cpp"

Game* game;
float deltaTime = 1;

int physTicks = 0;

intern void GameEventListener(const Event* event, void* data) {
	switch (event->type) {
		case Event_Quit:
		{
			Log("Quitting");
			game->isRunning = false;
			break;
		}
        
		case Event_Mouse:
		{
			if (event->mouse.type == MOUSE_MOVE)
			{
				game->gui.mouseX = event->mouse.posX;
				game->gui.mouseY = event->mouse.posY;
			}
            
			if (event->mouse.type == Mouse_Scroll)
				game->gui.scrollDelta += event->mouse.scrollDelta;
            
			// ~Temp i dont think we want the gui to work only with mouse 1 (left mouse button) but we should be able to differentiate instead of merge them like before (do this later tho, when we need it) 
			if (event->mouse.buttonId == 0 && event->mouse.type == MOUSE_BUTTON_DOWN) 	Gui_SetMouseButtonState(&game->gui, true);
			if (event->mouse.buttonId == 0 && event->mouse.type == MOUSE_BUTTON_UP) 	Gui_SetMouseButtonState(&game->gui, false);
            
			break;
		}
        
		case Event_Keyboard:
		{
			if (event->key.type == KEY_EVENT_DOWN)
			{
				Assert(game->gui.keyInputHead < 1024);
				game->gui.keyInputBuffer[game->gui.keyInputHead++] = event->key.key;
			}
            
			{
				bool down = event->key.type == KEY_EVENT_DOWN;
				
				if (event->key.key == K_LSHIFT) game->gui.lshift = down;
			}
            



            if (event->key.type == KEY_EVENT_DOWN)
            {
            	if (event->key.key == 'p')
            	{
					DeleteWorld(&game->world);
					if (!LoadWorld("map/world", &game->world))
						Log("You failed!");
					else 
					{
						For (game->world.sectors.keys)
							LoadSector(&game->world, *it);
					}
            	}
            }

			break;
		}
	}
}



intern bool isEditMode = true;

#include "telemetry.h"

void RunGame() {
	defer(FreeAllocators());
    
    
	game = (Game*)PushAlloc(sizeof(Game), &GetAllocators()->engineArena);
    
	Log("[game] Innit");
    
	game->eventBus = MakeEventBus();
	defer(DestroyEventBus(&game->eventBus));
	game->eventBus.AddEventListener(GameEventListener);
    
	InitResourceManager();
	defer(DestroyResourceManager());
    
    
	// Surface creation
	SurfaceSpawnInfo spawnInfo;
    
	constexpr u32 Window_Width = 1920, Window_Height = 1080;
    
	spawnInfo.title = "Never Engine";
	spawnInfo.width = Window_Width;
	spawnInfo.height = Window_Height;

	spawnInfo.eventBus = &game->eventBus;
    
	// bruh this was supposed to be nocheckin how did i check this into git
	spawnInfo.rendererType = RENDERER_VULKAN;

	game->surface = SpawnSurface(&spawnInfo, Engine_Arena);
	defer(DeleteSurface(game->surface));
    
	// Renderer creation
	{
		RendererSpawnInfo spawnInfo;
		spawnInfo.spawner = MakeRendererSpawner(game->surface);
        
		// cant set the gpu on opengl (or d3d11 yet) so this code is needed to not mess everything up lol
        
		Log("[gfx] Spawner Flags: %lu", spawnInfo.spawner.flags);
        
		for (size_t it = 0; it < spawnInfo.spawner.graphicsProcessors.size; it++) {
			GraphicsProcessor proc = spawnInfo.spawner.graphicsProcessors.data[it];
			Log("[gfx] Device Found: id:%lu - %s", it, proc.deviceName);
		}
        
		if (spawnInfo.spawner.flags & RENDERER_SPAWNER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY) {
			u32 procId = 0;
			// if (spawnInfo.spawner.graphicsProcessors.size > 1) {
			// 	printf("Choose a GPU by id: ");
			// 	scanf("%u", &procId);
			// }
			spawnInfo.chosenGraphicsProcessor = spawnInfo.spawner.graphicsProcessors.data[procId];
		}
        
		game->renderer = SpawnRenderer(&spawnInfo, Engine_Arena);
	}
	defer(DeleteRenderer(game->renderer));
    
    
	// Render system creation
	InitRenderSystem(&game->renderSystem, game->renderer); // ~Todo allocate this on the game memory
	defer(DeleteRenderSystem(&game->renderSystem));
    
	// Gui
	if (!InitGui(&game->gui, game->renderer, Window_Width, Window_Height)) {
		FatalError("[game] Failed to init GUI!");
	}
	defer(DestroyGui(&game->gui));
    
	// Init physics
	InitPhysics();
	defer(DestroyPhysics());


	// Init audio
	StartAudioThread();
	defer(StopAudioThread());
	



	{ // Init game
		if (!LoadWorld("map/world", &game->world))
			game->world = CreateWorld();
		else
		{
			// For (game->world.sectors.keys)
			// 	LoadSector(&game->world, *it);
			

			for (int y=-1; y<=0; ++y)
				for (int x=-1; x<=0; ++x)
					LoadSector(&game->world, { .x = x, .z = y });
		}
	}
	defer(DeleteWorld(&game->world));



	RendererHandle tex = LoadTexture("gui.png")->textureHandle;
    
	GuiButtonStyle style { };
	style.normalBg.type = GuiStyleProp_NineSlice;
	style.normalBg.nineSlice.image.texture = tex;
    
	style.normalBg.nineSlice.image.x = 374;
	style.normalBg.nineSlice.image.y = 500 - 421;
	style.normalBg.nineSlice.image.width = 499 - 374;
	style.normalBg.nineSlice.image.height = 421 - 391;
    
	style.normalBg.nineSlice.top    = 
    style.normalBg.nineSlice.left   = 
    style.normalBg.nineSlice.right  = 
    style.normalBg.nineSlice.bottom = 15; 
	
    
	style.hoverBg.type = GuiStyleProp_Color;
	style.hoverBg.color = v4(1,0,1,1);
    
	style.pressedBg.type = GuiStyleProp_Color;
	style.pressedBg.color = v4(1,0,0,1);
    
	style.normalFgColor  = v4(0, 0, 0, 1);
	style.hoverFgColor 	 = v4(0, 0.5, 0, 1);
	style.pressedFgColor = v4(0, 1, 0, 1);
    
	
	GuiFont font;
	LoadGuiFont(&game->gui, "fonts/NotoSans-Regular.ttf", &font);
	defer(FreeGuiFont(&game->gui, &font));
    
	style.font = &font;
	
	game->gui.style.button = style;
    
    
    
	GuiTitleBarStyle titleBarStyle { };
	
	titleBarStyle.bg.type = GuiStyleProp_Image;
	titleBarStyle.bg.image.texture = tex;
	titleBarStyle.bg.image.x 	   = 0;
	titleBarStyle.bg.image.y 	   = 499 - 210;
	titleBarStyle.bg.image.width   = 123;
	titleBarStyle.bg.image.height  = 210 - 190;
	
	titleBarStyle.fgColor = v4(1, 1, 1, 1);
	titleBarStyle.font 	  = &font;
    
	game->gui.style.titleBar = titleBarStyle;
    
    
    
	GuiTextboxStyle textboxStyle { };
    
	textboxStyle.paddingLeft = 10;
    
	textboxStyle.bg.type = GuiStyleProp_Image;
	textboxStyle.bg.image.texture = tex;
	textboxStyle.bg.image.x 	   = 0;
	textboxStyle.bg.image.y 	   = 499 - 210;
	textboxStyle.bg.image.width   = 123;
	textboxStyle.bg.image.height  = 210 - 190;
    
	textboxStyle.fgColor = v4(1, 1, 1, 1);
	textboxStyle.font 	 = &font;
    
	game->gui.style.textbox = textboxStyle;
    
    
    
	GuiListViewStyle listViewStyle { };
    
	listViewStyle.bg.type = GuiStyleProp_Color;
	listViewStyle.bg.color = v4(0.3, 0.3, 0.3, 1);
	
	listViewStyle.fgColor = v4(0, 0, 0, 1);
	listViewStyle.hoverFgColor = v4(1, 1, 1, 1);
	listViewStyle.pressedFgColor = v4(0, 0, 0, 1);
	listViewStyle.selectedFgColor = v4(0, 0, 0, 1);
    
	listViewStyle.entryBg.type = GuiStyleProp_Color;
	listViewStyle.entryBg.color = v4(0.5);
	
	listViewStyle.entryHoverBg.type = GuiStyleProp_Color;
	listViewStyle.entryHoverBg.color = v4(0.2);
	
	listViewStyle.entryPressedBg.type = GuiStyleProp_Color;
	listViewStyle.entryPressedBg.color = v4(0.9);
	
	listViewStyle.entrySelectedBg.type = GuiStyleProp_Color;
	listViewStyle.entrySelectedBg.color = v4(0.9);
	
    
	listViewStyle.entryHeight = 30;
	listViewStyle.entryPadding = 10;
	listViewStyle.font 	  = &font;
    
	game->gui.style.listView = listViewStyle;
    
    
	GuiSliderStyle sliderStyle { };
	sliderStyle.sliderBar.type = GuiStyleProp_Color;
	sliderStyle.sliderBar.color = v4(0.9);
	sliderStyle.sliderDot.type = GuiStyleProp_Color;
	sliderStyle.sliderDot.color = v4(1,0,1,1);
	// ~Refactor padding?
    
	game->gui.style.slider = sliderStyle;
    
    
	EditorInit();
	defer(EditorDestroy());
	
    
	// measure frametime
	constexpr int Frame_Time_Avg_Window = 16;
	int frameTimeHistoryHead = 0;
	float frameTimeHistory[Frame_Time_Avg_Window] { };
    
	// Game loop
	u64 previousTime = GetTicks();
    
	game->isRunning = true;
	while (game->isRunning)
	{		
		Telemetry prevTelemetry = telemetry;
		ResetTelemetry();
        
		telemetry.mem_frameArenaUsage = GetAllocators()->frameArena.used;
		// reset the frame allocator
		GetAllocators()->frameArena.used = 0;
        
		
        
		game->surface->Update();
        
		ResetGizmos();
		BeginGui(&game->gui);
        
		if (Gui_Button(&game->gui, isEditMode ? "Close Editor" : "Open Editor", 200, 100, 200, 50))
			isEditMode = !isEditMode;
        
		if (isEditMode)
		{
			EditorUpdate();
		}
		else
		{
			{
				World* world = &game->world;
				
				ForIt (world->entityPools, pool)
				{
					for (EntityPoolIt it = EntityPoolBegin(*pool); EntityPoolItValid(it); it = EntityPoolNext(it))
					{
						Entity* actor = GetEntityFromPoolIt(it);
                        
						if (actor->rigidbody)
						{
							SetXform(actor->rigidbody, actor->xform);
						}
					}
				}
                
				if (deltaTime > 0)
				{
					local_persist float accum = 0;
					accum += deltaTime;
					physTicks = 0;
					while (accum > Physics_Timestep)
					{
						accum -= Physics_Timestep;
						UpdatePhysicsWorld(&world->physicsWorld, Physics_Timestep);
					
						physTicks++;
					}
				}
                
				ForIt (world->entityPools, pool)
				{
					for (EntityPoolIt it = EntityPoolBegin(*pool); EntityPoolItValid(it); it = EntityPoolNext(it))
					{
						Entity* actor = GetEntityFromPoolIt(it);
                        
						if (actor->rigidbody)
						{
							actor->xform = GetXform(actor->rigidbody);
						}
					}
				}
			}
            

			{ // streaming test ~Temp
				EntityRegEntry entry;
				GetEntityEntry(Player, &entry);

				EntityPool pool;
				GetWorldEntityPool(&game->world, entry.typeId, &pool);
				Entity* palyer = GetEntityFromPoolIt(EntityPoolBegin(pool));
				
				Vec3 p = palyer->xform.position;
				WorldSectorId pSec = GetSectorId(p);
				// printf("sector %d,%d!\n", pSec.x, pSec.z);

				for (int y=-1; y<=1; y++)
				{
					for (int x=-1; x<=1; x++)
					{
						WorldSectorId at = { pSec.x + x, pSec.z + y };
						
						if (HasKey(game->world.sectors, at))
							LoadSector(&game->world, at);
					}
				}

				ForIdx (game->world.sectors.keys, idx)
				{
					auto sectorKey = &game->world.sectors.keys[idx];
					if (game->world.sectors.values[idx].loaded &&
					   ((sectorKey->x < pSec.x - 1) ||
						(sectorKey->x > pSec.x + 1) ||
						(sectorKey->z < pSec.z - 1) ||
						(sectorKey->z > pSec.z + 1))
					)
					{
						UnloadSector(&game->world, *sectorKey);
					}
						
				}
			} 

			UpdateWorld(&game->world);
		}
        
        
        
        
		DrawPhysicsDebug(&game->world.physicsWorld);
        
		// f
		float avgFrametime = 0;
		for (int i = 0; i < Frame_Time_Avg_Window; ++i) avgFrametime += frameTimeHistory[i];
		avgFrametime /= Frame_Time_Avg_Window;
		Gui_DrawText(&game->gui, TPrint("polygons: %u    time: %f", prevTelemetry.gfx_polygonCount, avgFrametime * 1000), 100, 400, GuiTextAlign_Left, &font, v4(1));
		EndGui(&game->gui);
        
        
		// update audio
		UpdateAudio();
		
        
		auto renderInfo = game->renderer->BeginRender(game->renderSystem.swapchain);
        
		if (isEditMode)
		{
			EditorRender(&game->renderSystem, renderInfo);
		}
		else
		{
			// Render the world
			RenderWorld(&game->renderSystem, renderInfo, &game->world);
		}
        
		game->renderer->EndRender(game->renderSystem.swapchain);
        
        
		// Calculate delta time
		u64 time = GetTicks();
		deltaTime = (time - previousTime) / 1000.0;
		previousTime = time;
		
		frameTimeHistory[frameTimeHistoryHead] = deltaTime;
		frameTimeHistoryHead++;
		frameTimeHistoryHead %= Frame_Time_Avg_Window;
	}
    
    
	// deinit
	Log("[game] Quit");
}






// startup

#ifndef PLATFORM_WINDOWS
int main()
{
	RunGame();
	
	return 0;
}
#else

#include <Windows.h>
int WINAPI WinMain(
                   HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nShowCmd)
{
#ifndef BUILD_DIST
	// Windows!! Why u gotta be like this????
    AllocConsole(); 
	freopen("CONOUT$", "w", stdout);
#endif
	
    RunGame();
	
	return 0;
}
#endif