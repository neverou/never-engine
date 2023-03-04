#pragma once

#include "gfx.h"
#include "gui.h"

void EditorInit();
void EditorDestroy();

void EditorUpdate();
void EditorRender(RenderSystem* renderSystem, RenderInfo renderInfo);
bool ModelBrowser(Gui* gui, String* model);