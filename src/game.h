#pragma once

#include "event.h"
#include "renderer.h"
#include "surface.h"

#include "gui.h"

#include "gfx.h"

#include "memory.h"

#include "world.h"

#include "physics.h"

struct Game {
    bool isRunning;
    
    EventBus eventBus;
    
    Surface* surface;
    Renderer* renderer;

    RenderSystem renderSystem;

	Gui gui;
    World world;
};

extern Game* game;
extern float deltaTime;

extern int physTicks;

void RunGame();
