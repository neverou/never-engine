#include "editor.h"

#include "game.h"
#include "gui.h"
#include "gizmos.h"
#include "maths.h"
#include "allocators.h"

#include "resource.h"


enum EditorUniversalTool
{
	EditorUniversalTool_Translate = 0, 
	EditorUniversalTool_Rotate,
	EditorUniversalTool_Scale,
    
	EditorUniversalTool_Terrain,
};



struct Editor_PropertyPanelField
{
	String textValue;
};


struct
{
    EditorUniversalTool tool = EditorUniversalTool_Translate;
    World world;
    
    struct
    {
        Xform xform;
        Mat4 projectionMatrix;
        Vec2 rotation;
    }
    camera;
    
    DynArray<EntityId> selectedEntities;
    GuiFont uiFont; // ~Refactor This should be removed ASAP
    
    struct
    {
        // mouse
        float mouseMoveX;
        float mouseMoveY;
        
        bool mbDown[MB_Count];
        bool mbHeld[MB_Count];
        bool mbUp  [MB_Count];
        //
        
        bool keyDown[Keycode_Count];
        bool keyHeld[Keycode_Count];
        bool keyUp  [Keycode_Count];
    } input;
    
    struct
    {
        bool init = false;
        DynArray<Editor_PropertyPanelField> fields;
    } selectData;
    
    struct
    {
        DynArray<SArray<u8>> actors;
    } copyBuffer;
} editor;



intern void EditorEventHandler(const Event* event, void* data) {
	if (event->type == Event_Mouse && event->mouse.type == MOUSE_MOVE) {
        editor.input.mouseMoveX += event->mouse.moveDeltaX;
        editor.input.mouseMoveY += event->mouse.moveDeltaY;
    }
    
    if (event->type == Event_Mouse)
    {
        switch (event->mouse.type)
        {
            case MOUSE_BUTTON_DOWN: 
            { 
                editor.input.mbDown[event->mouse.buttonId] = true;
                editor.input.mbHeld[event->mouse.buttonId] = true;
            } break;
            
            case MOUSE_BUTTON_UP: { 
                editor.input.mbUp[event->mouse.buttonId]   = true; 
                editor.input.mbHeld[event->mouse.buttonId] = false; 
            } break;
        }
    }
    
    if (event->type == Event_Keyboard)
    {
        editor.input.keyHeld[event->key.key] = event->key.type == KEY_EVENT_DOWN;
        if (event->key.type == KEY_EVENT_DOWN)
            editor.input.keyDown[event->key.key] = true;
        if (event->key.type == KEY_EVENT_UP)
            editor.input.keyUp[event->key.key]   = true;
    }
}

void EditorInit()
{
	LoadGuiFont(&game->gui, "fonts/NotoSans-Regular.ttf", &editor.uiFont);
    
    editor.world = CreateWorld();
    
    // ~Hack: save time for now by loading the default world file
	LoadWorld("map/world", &editor.world);
    
	editor.camera.xform = CreateXform();
	editor.camera.projectionMatrix = PerspectiveMatrix(70, 1920.0 / 1080.0, 0.1, 1000);
    
    game->eventBus.AddEventListener(EditorEventHandler);
    
    editor.selectedEntities = MakeDynArray<EntityId>();
    
    // Init copy buffer
    editor.copyBuffer.actors = MakeDynArray<SArray<u8>>();
}

void EditorDestroy()
{	
	FreeGuiFont(&game->gui, &editor.uiFont);
    
    
    FreeDynArray(&editor.selectedEntities);
    
    // Free copy buffer
    For (editor.copyBuffer.actors)
        FreeArray(it);
    FreeDynArray(&editor.copyBuffer.actors);
}

intern void SelectActor(EntityId EntityId, bool add)
{
	if (add && ArrayFind(&editor.selectedEntities, EntityId) != -1) // actor already selected!
		return;
    
	if (!add) ArrayClear(&editor.selectedEntities);
	ArrayAdd(&editor.selectedEntities, EntityId);
    
	// property panel
	{
		Entity* actor = GetWorldEntity(&editor.world, editor.selectedEntities[0]); // ~Refactor @@MultiSelect
        
		if (!editor.selectData.init)
		{
			editor.selectData.init = true;
			editor.selectData.fields = MakeDynArray<Editor_PropertyPanelField>();
		}
        
		For (editor.selectData.fields)
		{
			FreeString(&it->textValue);
		}
		ArrayClear(&editor.selectData.fields);
		
        
        
		auto* data = actor->GetDataLayout();
		if (data)
			while (data->type != 0)
        {
            Editor_PropertyPanelField field = {};
            field.textValue = MakeString();
            
            ArrayAdd(&editor.selectData.fields, field);
            
            data++;
        }
	}
}


// search util

intern bool MatchSearchResult(StringView query, StringView result)
{
	if (query.length > result.length) return false;

	for (int i = 0; i < result.length - query.length; i++)
	{
		bool match = true;
		for (int j = 0; j < query.length; j++)
		{
			match &= result[i] == query[j];
		}

		if (match) return true;
	}

	return false;
}



#include "terrain.h"

// Terrain stuff

intern void EditorCreateTerrain(World* world, int chunkX, int chunkZ)
{
	if (!LookupTerrain(world, chunkX, chunkZ))
	{
		Xform xform = CreateXform();
		Terrain* actor = CreateTerrain(world, chunkX, chunkZ);
	}
}

intern float TerrainRaycast(World* world, Vec3 o, Vec3 d)
{
	float maxDist = (o.y + 15) / -d.y;
	for (float t = 0; t < maxDist; t += maxDist / 100)
	{
		Vec3 p = o + d * v3(t);
        
		if (LookupTerrain(world, (int)Floor(p.x / float(Terrain_ChunkSize)), (int)Floor(p.z / float(Terrain_ChunkSize))) && TerrainSampleHeight(world, p.x, p.z) > p.y)
		{
			return t;
		}
	}
	return -1;
}


enum TerrainToolMode
{
	TerrainTool_Create,
	TerrainTool_Brush,
};

enum TerrainToolBrush
{
	TerrainBrush_Add,
	TerrainBrush_Sub,
	TerrainBrush_Smooth,
	TerrainBrush_Flatten,
};

intern void DoTerrainTool(Gui* gui)
{
	local_persist TerrainToolMode terrainTool = TerrainTool_Create;
	local_persist TerrainToolBrush terrainBrush = TerrainBrush_Add;
	local_persist float terrainBrushRadius = 5;
	local_persist float terrainBrushIntensity = 20;
	
    
	Mat4 viewMatrix = Inverse(XformMatrix(editor.camera.xform));
    
	Vec2 mouse = v2(gui->mouseX / (gui->width - 1), gui->mouseY / (gui->height - 1));
	Vec3 mouseRayDir = Normalize(Mul( Normalize(Unproject( v3(mouse.x, mouse.y, 1), editor.camera.projectionMatrix )), 0, Inverse(viewMatrix)));
    
	if (editor.tool == EditorUniversalTool_Terrain)
	{
		// terrain tool panel
		float width = 300;
		float panelHeight = 400;
		Panel panel = MakePanel(100, gui->height / 2 + panelHeight / 2, width);
        
		Gui_DrawRect(gui, panel.atX, panel.atY - panelHeight, width, panelHeight, v4(0.1));
        
		PanelRow(&panel, 32);
		Gui_TitleBar(gui, "Terrain Tools", panel.atX, panel.atY, panel.width, 32);
        
		PanelRow(&panel, 16); // padding
        
		PanelRow(&panel, 32);
		if (Gui_Button(gui, "Create", panel.atX, panel.atY, width / 2, 32))
			terrainTool = TerrainTool_Create;
		panel.atX += width / 2;
		if (Gui_Button(gui, "Brush", panel.atX, panel.atY, width / 2, 32))
			terrainTool = TerrainTool_Brush;
        
		// ~Refactor move sizes and stuff to constants at the beginning of this function
        
		PanelRow(&panel, 64);
		if (terrainTool == TerrainTool_Brush)
		{
			Gui_TitleBar(gui, "Brush Settings", panel.atX + panel.width / 4, panel.atY, panel.width / 2, 32);
            
			PanelRow(&panel, 32);
			float rowWidth = 32 + 32 + 100 + 100;
			panel.atX += width / 2 - rowWidth / 2;
			if (Gui_Button(gui, "+", panel.atX, panel.atY, 32, 32)) terrainBrush = TerrainBrush_Add;
			panel.atX += 32;
			if (Gui_Button(gui, "-", panel.atX, panel.atY, 32, 32)) terrainBrush = TerrainBrush_Sub;
			panel.atX += 32;
			if (Gui_Button(gui, "Smooth", panel.atX, panel.atY, 100, 32)) terrainBrush = TerrainBrush_Smooth;
			panel.atX += 100;
			if (Gui_Button(gui, "Flatten", panel.atX, panel.atY, 100, 32)) terrainBrush = TerrainBrush_Flatten;
			PanelRow(&panel, 32); // padding
			panel.atX += 16;
            
			StringView currentBrush = "";
			if (terrainBrush == TerrainBrush_Add) 		currentBrush = "Add";
			if (terrainBrush == TerrainBrush_Sub) 		currentBrush = "Sub";
			if (terrainBrush == TerrainBrush_Smooth) 	currentBrush = "Smooth";
			if (terrainBrush == TerrainBrush_Flatten) 	currentBrush = "Flatten";
			Gui_DrawText(gui, TPrint("Current Brush: %s", currentBrush.data), panel.atX, panel.atY, GuiTextAlign_Left, &editor.uiFont, v4(0));
            
			PanelRow(&panel, 16); // padding
            
			{ // radius
				PanelRow(&panel, 32);
				Gui_DrawText(gui, "Radius: ", panel.atX, panel.atY, GuiTextAlign_Left, &editor.uiFont, v4(0));
				panel.atX += width / 2;
                
				local_persist String inputBrushRadius = MakeString();
				// ~Refactor new GUI inputs (slider, number box, etc)
				auto interact = Gui_Textbox(gui, &inputBrushRadius, 0, panel.atX, panel.atY, width / 2, 32);
				if (interact.enter)
				{
					sscanf(inputBrushRadius.data, "%f", &terrainBrushRadius);
				}
				else if (!interact.active)
				{
					inputBrushRadius.Update(TPrint("%.2f", terrainBrushRadius));
				}
			}
            
			{ // intensity
				PanelRow(&panel, 32);
				Gui_DrawText(gui, "Intensity: ", panel.atX, panel.atY, GuiTextAlign_Left, &editor.uiFont, v4(0));
				panel.atX += width / 2;
                
				local_persist String inputBrushIntensity = MakeString();
				// ~Refactor new GUI inputs (slider, number box, etc)
				// auto interact = Gui_Textbox(gui, &inputBrushIntensity, 0, panel.atX, panel.atY, width / 2, 32);
				// if (interact.enter)
				// {
				// 	sscanf(inputBrushIntensity.data, "%f", &terrainBrushIntensity);
				// }
				// else if (!interact.active)
				// {
				// 	inputBrushIntensity.Update(TPrint("%.2f", terrainBrushIntensity));
				// }
				
				Gui_SliderFloat(gui, panel.atX, panel.atY, width / 2, 32, &terrainBrushIntensity, 0, 100);
			}
		}
        
        
		CompletePanel(&panel);
        
		switch (terrainTool)
		{
			case TerrainTool_Create:
			{
				Vec3 rayPlaneIsct = mouseRayDir * v3(editor.camera.xform.position.y / -mouseRayDir.y) + editor.camera.xform.position;
                
				bool hover = false;
				int hoverX = 0;
				int hoverZ = 0;
                
                
				local_persist bool dragging = false;
				local_persist int downX = 0;
				local_persist int downZ = 0;
                
                
                
				int cameraOverChunkX = Floor(editor.camera.xform.position.x / float(Terrain_ChunkSize));
				int cameraOverChunkZ = Floor(editor.camera.xform.position.z / float(Terrain_ChunkSize));
                
				for (int i = cameraOverChunkX - 5; i < cameraOverChunkX + 5; i++)
				{
					for (int j = cameraOverChunkZ - 5; j < cameraOverChunkZ + 5; j++)
					{
						if (Abs(rayPlaneIsct.x - (i + 0.5) * Terrain_ChunkSize) < Terrain_ChunkSize / 2 && Abs(rayPlaneIsct.z - (j + 0.5) * Terrain_ChunkSize) < Terrain_ChunkSize / 2)
						{
							hover = true;
							hoverX = i;
							hoverZ = j;
						}
					}
				}
                
				int minX = (downX < hoverX) ? downX : hoverX;
				int minZ = (downZ < hoverZ) ? downZ : hoverZ;
				int maxX = (downX > hoverX) ? downX : hoverX;
				int maxZ = (downZ > hoverZ) ? downZ : hoverZ;
                
				for (int i = cameraOverChunkX - 5; i < cameraOverChunkX + 5; i++)
				{
					for (int j = cameraOverChunkZ - 5; j < cameraOverChunkZ + 5; j++)
					{
						Vec3 color = v3(0, 0.3, 1);
                        
						if (LookupTerrain(&editor.world, i, j) != 0) continue; // if theres already terrain there dont show an add button
                        
						if (dragging)
						{
							if (i >= minX && i <= maxX && j >= minZ && j <= maxZ)
							{
								color = v3(1, 1, 1);
							}
						}
                        
						if (hover && hoverX == i && hoverZ == j)
							color = v3(0.5, 1, 0);
                        
						GizmoCube(v3((i + 0.5) * Terrain_ChunkSize, 0, (j + 0.5) * Terrain_ChunkSize), v3(Terrain_ChunkSize / 4.0, 0.1, Terrain_ChunkSize / 4.0), color);
					}
				}
                
                
				if (hover && editor.input.mbDown[MB_Left] && gui->hotId == GuiId_None)
				{
					dragging = true;
					downX = hoverX;
					downZ = hoverZ;
				}
				if (dragging && hover && editor.input.mbUp[MB_Left] && gui->hotId == GuiId_None)
				{
					// create a terrain chunk
					for (int z = minZ; z <= maxZ; z++)
						for (int x = minX; x <= maxX; x++)
                        EditorCreateTerrain(&editor.world, x, z);
					dragging = false;
				}
                
				break;
			}
            
            
			case TerrainTool_Brush:
			{
				// draw raycast
				float t = TerrainRaycast(&editor.world, editor.camera.xform.position, mouseRayDir);
				if (t > 0)
				{
					Vec3 p = editor.camera.xform.position + v3(t) * mouseRayDir;
                    
					if (editor.input.mbHeld[MB_Left] && gui->hotId == GuiId_None)
					{
						float avgHeight = 0;
						int samples = 0;
                        
						int xMin = Floor(p.x - terrainBrushRadius);
						int xSize = Floor(p.x + terrainBrushRadius) + 1 - xMin;
						int zMin = Floor(p.z - terrainBrushRadius);
						int zSize = Floor(p.z + terrainBrushRadius) + 1 - zMin;
                        
						Array<float> brushArea = TerrainReadHeightArea(&editor.world, xMin, zMin, xSize, zSize);
                        
						for (int x = 0; x < xSize; x++)
							for (int z = 0; z < zSize; z++) 
                        {
                            float h = brushArea[x + z * xSize];
                            avgHeight += h;
                            samples++;
                        }
                        
						avgHeight /= samples;
                        
						for (int x = 0; x < xSize; x++)
							for (int z = 0; z < zSize; z++) 
                        {
                            float h = brushArea[x + z * xSize];
                            
                            float dist = Max(1 - Sqrt((xSize / 2 - x) * (xSize / 2 - x) + (zSize / 2 - z) * (zSize / 2 - z)) / terrainBrushRadius, 0);
                            float kernel = -Cos(dist * 180) * 0.5 + 0.5;
                            
                            if (terrainBrush == TerrainBrush_Add)
                            {
                                h += kernel * terrainBrushIntensity * deltaTime;
                            }
                            else if (terrainBrush == TerrainBrush_Sub)
                            {
                                h -= kernel * terrainBrushIntensity * deltaTime;
                            }
                            else if (terrainBrush == TerrainBrush_Flatten)
                            {
                                float v = kernel * terrainBrushIntensity * deltaTime;
                                if (v > 1) v = 1;
                                h += v * (avgHeight - h);
                            }
                            
                            
                            h = Max(h, 0);
                            brushArea[x + z * xSize] = h;
                        }
                        
						TerrainWriteHeightArea(&editor.world, xMin, zMin, xSize, zSize, brushArea);
						ValidateTerrains(&editor.world);
					}
                    
					float epsilon = 0.1;
					float posY = TerrainSampleHeight(&editor.world, p.x, p.z);
					Vec3 dx = v3(epsilon, TerrainSampleHeight(&editor.world, p.x + epsilon, p.z) - posY, 0);
					Vec3 dz = v3(0, TerrainSampleHeight(&editor.world, p.x, p.z + epsilon) - posY, epsilon);
                    
					Vec3 n = Normalize(Cross(dz, dx));
					GizmoDisc(p + n * v3(2), n, terrainBrushRadius * 0.95, terrainBrushRadius, v3(1,1,0));
				}
                
				break;
			}
            
		}
        
		
	}
}


//

intern void ApplyTranslation(Array<EntityId> actors, Vec3 translation)
{
	For (actors)
	{
		Entity* actor = GetWorldEntity(&editor.world, *it);
		actor->xform.position = actor->xform.position + translation;
	}
}

intern void ApplyScale(Array<EntityId> actors, Vec3 scale)
{
	For (actors)
	{
		Entity* actor = GetWorldEntity(&editor.world, *it);
		actor->xform.scale = actor->xform.scale + scale;
	}
}

intern void ApplyRotation(Array<EntityId> actors, Mat4 rotation)
{
	For (actors)
	{
		Entity* actor = GetWorldEntity(&editor.world, *it);
		actor->xform.rotation = Mul(rotation, actor->xform.rotation);
	}
}


void EditorUpdate()
{
	auto* gui = &game->gui;

	auto* world = &editor.world;

	UpdateSectors(world);


	// {
	// 	auto items = MakeDynArray<StringView>(0, Frame_Arena);

	// 	static int a = 0;
	// 	static float b = 0;

	// 	For (world->sectors.keys)
	// 	{
	// 		auto sector = &world->sectors[*it];
	// 		ForIt (sector->entities, entIt)
	// 		{
	// 			StringView s = TPrint("%d,%d -> %lu", it->x, it->z, *entIt);
	// 			ArrayAdd(&items, s);
	// 		}
	// 	}

	// 	Gui_ListView(gui, 400, gui->height - 400, 400, 400, items, &a, &b);
	// }



	{ // camera mover
		Mat4 cameraOrientation = XformMatrix(editor.camera.xform);
		Vec3 cameraForward = OrientForward(cameraOrientation);
		Vec3 cameraRight = OrientRight(cameraOrientation);
		Vec3 cameraUp = OrientUp(cameraOrientation);

		if (editor.input.mbHeld[MB_Right])
		{
			editor.camera.rotation.x += editor.input.mouseMoveX / 8.0;
			editor.camera.rotation.y -= editor.input.mouseMoveY / 8.0;

			// ~Todo editor tweaks menu or something to adjust this
			// maybe a code constant?
			float moveSpeed = editor.input.keyHeld[K_LSHIFT] ? 100 : 30;
            
			Vec3 moveDir = v3(((s32)editor.input.keyHeld['d'] - (s32)editor.input.keyHeld['a']) * moveSpeed,
                              ((s32)editor.input.keyHeld['e'] - (s32)editor.input.keyHeld['q']) * moveSpeed,
                              ((s32)editor.input.keyHeld['w'] - (s32)editor.input.keyHeld['s']) * moveSpeed);
            
			Vec3 moveDelta = ((v3(moveDir.x) * cameraRight) + 
                              (v3(moveDir.y) * cameraUp) + 
                              (v3(moveDir.z) * cameraForward)) * v3(deltaTime);
			editor.camera.xform.position += moveDelta;
		}
		else if (editor.input.mbHeld[MB_Middle])
		{
			float panSpeed = 1.0 / 20.0;
			Vec3 pan = v3(editor.input.mouseMoveX) * cameraRight + v3(editor.input.mouseMoveY) * cameraUp;
			editor.camera.xform.position += pan * v3(panSpeed);
		}
        
		editor.camera.rotation.y = Clamp(editor.camera.rotation.y, -90, 90);
		editor.input.mouseMoveY = 0;
		editor.input.mouseMoveX = 0;
        
		editor.camera.xform.rotation = Mul(RotationMatrixAxisAngle(v3(0,1,0), editor.camera.rotation.x), RotationMatrixAxisAngle(v3(1,0,0), editor.camera.rotation.y));
	}
    
    
	{ // universal tool changer
		int buttons = 4;
		float buttonSize = 50;
        
		float width = buttons * buttonSize;
		Panel panel = MakePanel(gui->width / 2 - width / 2, 100, width);
        
		if (Gui_Button(gui, "T", panel.atX, panel.atY, buttonSize, buttonSize)) editor.tool = EditorUniversalTool_Translate;
		panel.atX += buttonSize;
		if (Gui_Button(gui, "R", panel.atX, panel.atY, buttonSize, buttonSize)) editor.tool = EditorUniversalTool_Rotate;
		panel.atX += buttonSize;
		if (Gui_Button(gui, "S", panel.atX, panel.atY, buttonSize, buttonSize)) editor.tool = EditorUniversalTool_Scale;
		panel.atX += buttonSize;
		if (Gui_Button(gui, "W", panel.atX, panel.atY, buttonSize, buttonSize)) editor.tool = EditorUniversalTool_Terrain;
	}
    
	{ // transform gizmo on selected actor
		constexpr float Transform_Gizmo_Scale = 0.5;
        
        
		if (editor.selectedEntities.size > 0) {
			bool individualOrigins = false; // ~Todo @@EditorImprovement add a ui button for this
            
			Vec3 originOfTransform { };
            
			For (editor.selectedEntities)
            {
				Entity* actor = GetWorldEntity(&editor.world, *it);
				Assert(actor);
                
				Vec3 worldOrigin = Mul(v3(0), 1, EntityWorldXformMatrix(world, actor)); // ~Refactor make this a function
                
				originOfTransform = originOfTransform + actor->xform.position;
			}
			originOfTransform = originOfTransform / v3(editor.selectedEntities.size); 
            
            
			Vec3 o = originOfTransform; // ~Hack fix with parented entities (also the rotation)
			
			Mat4 cameraPose = XformMatrix(editor.camera.xform);
			Vec3 cameraForward = OrientForward(cameraPose);
			Vec3 camPos = v3(cameraPose.rows[0].w, cameraPose.rows[1].w, cameraPose.rows[2].w);
            
            
			float s = Dot(Sub(o, camPos), OrientForward(cameraPose)) * Transform_Gizmo_Scale;
            
			Vec3 xHead, yHead, zHead;
            
			// ~CleanUp compute xDir,yDir,zDir first and then multiply by gizmoScale instead of this
			// to get rid of the normalize()
            
			float gizmoScale = s * 0.3;
			xHead = Add(o, v3(gizmoScale,0,0));
			yHead = Add(o, v3(0,gizmoScale,0));
			zHead = Add(o, v3(0,0,gizmoScale));
            
            
			// ~Todo relative mode
            
			Vec3 xDir = Normalize(Sub(xHead, o));
			Vec3 yDir = Normalize(Sub(yHead, o));
			Vec3 zDir = Normalize(Sub(zHead, o));
            
            
			Mat4 viewMatrix = Inverse(XformMatrix(editor.camera.xform));
            
            
			// ~Refactor make this a function
			Vec2 mouse = v2(gui->mouseX / (gui->width - 1), gui->mouseY / (gui->height - 1));
			Vec3 mouseRayDir = Mul( Normalize(Unproject( v3(mouse.x, mouse.y, 1), editor.camera.projectionMatrix )), 0, Inverse(viewMatrix));
            
            
			switch (editor.tool)
			{
				case EditorUniversalTool_Translate:
				{
					constexpr u8 Gizmo_X = 1;
					constexpr u8 Gizmo_Y = 2;
					constexpr u8 Gizmo_Z = 3;
					local_persist u8 gizmoHands = 0;
					local_persist Vec3 prevLoc = v3(0);
					local_persist Vec4 initialPlane = v4(0);
                    
					if (gizmoHands == 0 || gizmoHands == Gizmo_X)
						GizmoArrow(o, xHead, s * 0.01, s * 0.025, s * 0.05, v3(1, 0, 0));
					if (gizmoHands == 0 || gizmoHands == Gizmo_Y)
						GizmoArrow(o, yHead, s * 0.01, s * 0.025, s * 0.05, v3(0, 1, 0));
					if (gizmoHands == 0 || gizmoHands == Gizmo_Z)
						GizmoArrow(o, zHead, s * 0.01, s * 0.025, s * 0.05, v3(0, 0, 1));
					// ~Temp?
					
					// casting these to vec2 because we dont care about the depth for this
					Vec2 oProj = v2(Project(Mul(o,     1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 xProj = v2(Project(Mul(xHead, 1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 yProj = v2(Project(Mul(yHead, 1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 zProj = v2(Project(Mul(zHead, 1, viewMatrix), editor.camera.projectionMatrix));	
                    
                    
					if (gui->mouseDown)
					{
						Vec2  xDiff = Sub(xProj, oProj);
						float xTrav = Dot(Normalize(xDiff), Sub(mouse, oProj)) / Length(xDiff);
						Vec2  yDiff = Sub(yProj, oProj);
						float yTrav = Dot(Normalize(yDiff), Sub(mouse, oProj)) / Length(yDiff);
						Vec2  zDiff = Sub(zProj, oProj);
						float zTrav = Dot(Normalize(zDiff), Sub(mouse, oProj)) / Length(zDiff);
                        
						Vec2 xPoint = Add(oProj, Mul( xDiff, v2(Clamp(xTrav, 0, 1)) ));
						Vec2 yPoint = Add(oProj, Mul( yDiff, v2(Clamp(yTrav, 0, 1)) ));
						Vec2 zPoint = Add(oProj, Mul( zDiff, v2(Clamp(zTrav, 0, 1)) ));

						float xDist = Length(Sub(xPoint, mouse));
						float yDist = Length(Sub(yPoint, mouse));
						float zDist = Length(Sub(zPoint, mouse));
						float minDist = Min(Min(xDist, yDist), zDist);
                        
						if (minDist < 0.05)
						{
							if (xDist < yDist)
							{
								if (xDist < zDist)
									gizmoHands = Gizmo_X;
								else
									gizmoHands = Gizmo_Z;
							}
							else
							{
								if (yDist < zDist)
									gizmoHands = Gizmo_Y;
								else
									gizmoHands = Gizmo_Z;
							}
						}
                        
						
						if (gizmoHands)
						{
							// calculate inital plane
							Vec3 planeCandidate0, planeCandidate1; 
							if (gizmoHands == Gizmo_X)
							{
								planeCandidate0 = yDir;
								planeCandidate1 = zDir;
							}
							if (gizmoHands == Gizmo_Y)
							{
								planeCandidate0 = xDir;
								planeCandidate1 = zDir;
							}
							if (gizmoHands == Gizmo_Z)
							{
								planeCandidate0 = xDir;
								planeCandidate1 = yDir;
							}
                            
                            
							if (Dot(planeCandidate0, cameraForward) > 0) planeCandidate0 = Sub(v3(0), planeCandidate0);
							if (Dot(planeCandidate1, cameraForward) > 0) planeCandidate1 = Sub(v3(0), planeCandidate1);
                            
							Vec3 planeNormal;
                            
							if (Dot(planeCandidate0, cameraForward) < Dot(planeCandidate1, cameraForward)) planeNormal = planeCandidate0;
							else 																		   planeNormal = planeCandidate1;
							
							initialPlane = v4(planeNormal, Dot(planeNormal, o));
                            
							float rayDist = (Dot(v3(initialPlane), camPos) - initialPlane.w) / -Dot(v3(initialPlane), mouseRayDir);
                            
							Vec3 rayHit = Add(camPos, Mul(mouseRayDir, v3(rayDist)));
                            
							prevLoc = rayHit;
						}
					}
					if (gui->mouseUp)
					{
						gizmoHands = 0;
					}
                    
					if (gizmoHands)
					{
						float rayDist = (Dot(v3(initialPlane), camPos) - initialPlane.w) / -Dot(v3(initialPlane), mouseRayDir);
						Vec3 rayHit = Add(camPos, Mul(mouseRayDir, v3(rayDist)));
                        
						Vec3 dir;
						if (gizmoHands == Gizmo_X) dir = xDir;
						if (gizmoHands == Gizmo_Y) dir = yDir;
						if (gizmoHands == Gizmo_Z) dir = zDir;
                        
						// Clamp the movement vector to only be the direction arrow that the user selected
						Vec3 diff = Mul(dir, v3(Dot(dir, Sub(rayHit, prevLoc))) );
						ApplyTranslation(editor.selectedEntities, diff);
                        
						prevLoc = rayHit;
					}
					break;
				}
                
                
				case EditorUniversalTool_Scale:
				{
					constexpr u8 Gizmo_X = 1;
					constexpr u8 Gizmo_Y = 2;
					constexpr u8 Gizmo_Z = 3;
					local_persist u8 gizmoHands = 0;
					local_persist Vec3 prevLoc = v3(0);
					local_persist Vec4 initialPlane = v4(0);
                    
                    
					if (gizmoHands == 0 || gizmoHands == Gizmo_X)
						GizmoHandle(o, xHead, s * 0.01, s * 0.025, v3(1, 0, 0));;
					if (gizmoHands == 0 || gizmoHands == Gizmo_Y)
						GizmoHandle(o, yHead, s * 0.01, s * 0.025, v3(0, 1, 0));
					if (gizmoHands == 0 || gizmoHands == Gizmo_Z)
						GizmoHandle(o, zHead, s * 0.01, s * 0.025, v3(0, 0, 1));
                    
                    
                    
                    
					Vec3 xDir = Normalize(Sub(xHead, o));
					Vec3 yDir = Normalize(Sub(yHead, o));
					Vec3 zDir = Normalize(Sub(zHead, o));
                    
					// ~Temp?
					
					// casting these to vec2 because we dont care about the depth for this
					Vec2 oProj = v2(Project(Mul(o,     1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 xProj = v2(Project(Mul(xHead, 1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 yProj = v2(Project(Mul(yHead, 1, viewMatrix), editor.camera.projectionMatrix));	
					Vec2 zProj = v2(Project(Mul(zHead, 1, viewMatrix), editor.camera.projectionMatrix));	
                    
                    
                    
                    
					if (gui->mouseDown)
					{
						Vec2  xDiff = Sub(xProj, oProj);
						float xTrav = Dot(Normalize(xDiff), Sub(mouse, oProj)) / Length(xDiff);
						Vec2  yDiff = Sub(yProj, oProj);
						float yTrav = Dot(Normalize(yDiff), Sub(mouse, oProj)) / Length(yDiff);
						Vec2  zDiff = Sub(zProj, oProj);
						float zTrav = Dot(Normalize(zDiff), Sub(mouse, oProj)) / Length(zDiff);
                        
						Vec2 xPoint = Add(oProj, Mul( xDiff, v2(Clamp(xTrav, 0, 1)) ));
						Vec2 yPoint = Add(oProj, Mul( yDiff, v2(Clamp(yTrav, 0, 1)) ));
						Vec2 zPoint = Add(oProj, Mul( zDiff, v2(Clamp(zTrav, 0, 1)) ));
                        
						float xDist = Length(Sub(xPoint, mouse));
						float yDist = Length(Sub(yPoint, mouse));
						float zDist = Length(Sub(zPoint, mouse));
						float minDist = Min(Min(xDist, yDist), zDist);
                        
						if (minDist < 0.05)
						{
							if (xDist < yDist)
							{
								if (xDist < zDist)
									gizmoHands = Gizmo_X;
								else
									gizmoHands = Gizmo_Z;
							}
							else
							{
								if (yDist < zDist)
									gizmoHands = Gizmo_Y;
								else
									gizmoHands = Gizmo_Z;
							}
						}
                        
						
						if (gizmoHands)
						{
							// calculate inital plane
							Vec3 planeCandidate0, planeCandidate1; 
							if (gizmoHands == Gizmo_X)
							{
								planeCandidate0 = yDir;
								planeCandidate1 = zDir;
							}
							if (gizmoHands == Gizmo_Y)
							{
								planeCandidate0 = xDir;
								planeCandidate1 = zDir;
							}
							if (gizmoHands == Gizmo_Z)
							{
								planeCandidate0 = xDir;
								planeCandidate1 = yDir;
							}
                            
                            
							if (Dot(planeCandidate0, cameraForward) > 0) planeCandidate0 = Sub(v3(0), planeCandidate0);
							if (Dot(planeCandidate1, cameraForward) > 0) planeCandidate1 = Sub(v3(0), planeCandidate1);
                            
							Vec3 planeNormal;
                            
							if (Dot(planeCandidate0, cameraForward) < Dot(planeCandidate1, cameraForward)) planeNormal = planeCandidate0;
							else 																		   planeNormal = planeCandidate1;
							
							initialPlane = v4(planeNormal, Dot(planeNormal, o));
                            
							float rayDist = (Dot(v3(initialPlane), camPos) - initialPlane.w) / -Dot(v3(initialPlane), mouseRayDir);
                            
							Vec3 rayHit = Add(camPos, Mul(mouseRayDir, v3(rayDist)));
                            
							prevLoc = rayHit;
						}
					}
					if (gui->mouseUp)
					{
						gizmoHands = 0;
					}
                    
					if (gizmoHands)
					{
						float rayDist = (Dot(v3(initialPlane), camPos) - initialPlane.w) / -Dot(v3(initialPlane), mouseRayDir);
						Vec3 rayHit = Add(camPos, Mul(mouseRayDir, v3(rayDist)));
                        
						Vec3 dir;
						if (gizmoHands == Gizmo_X) dir = xDir;
						if (gizmoHands == Gizmo_Y) dir = yDir;
						if (gizmoHands == Gizmo_Z) dir = zDir;
                        
						// Clamp the scale vector to only be the direction arrow that the user selected
						Vec3 diff = Mul(dir, v3(Dot(dir, Sub(rayHit, prevLoc))) );
						ApplyScale(editor.selectedEntities, diff);
                        
						prevLoc = rayHit;
					}
                    
					break;
				}
                
                
				case EditorUniversalTool_Rotate:
				{			
					constexpr u8 Gizmo_X = 1;
					constexpr u8 Gizmo_Y = 2;
					constexpr u8 Gizmo_Z = 3;
					local_persist u8 gizmoHands = 0;
					local_persist Vec3 initialLoc = v3(0);
                    
					if (Dot(xDir, cameraForward) > 0) xDir = Sub(v3(0), xDir);
					if (Dot(yDir, cameraForward) > 0) yDir = Sub(v3(0), yDir);
					if (Dot(zDir, cameraForward) > 0) zDir = Sub(v3(0), zDir);
					float ringRadius = gizmoScale * 0.9;
					float ringVisWidth = s * 0.007;
					float ringRange  = s * 0.05;
                    
					if (gizmoHands == 0 || gizmoHands == Gizmo_X) GizmoDisc(o, xDir, ringRadius - ringVisWidth, ringRadius + ringVisWidth, v3(1, 0, 0));
					if (gizmoHands == 0 || gizmoHands == Gizmo_Y) GizmoDisc(o, yDir, ringRadius - ringVisWidth, ringRadius + ringVisWidth, v3(0, 1, 0));
					if (gizmoHands == 0 || gizmoHands == Gizmo_Z) GizmoDisc(o, zDir, ringRadius - ringVisWidth, ringRadius + ringVisWidth, v3(0, 0, 1));
                    
					Vec4 planeX = v4(xDir, Dot(o, xDir));
					Vec4 planeY = v4(yDir, Dot(o, yDir));
					Vec4 planeZ = v4(zDir, Dot(o, zDir));
                    
					float xTrace = (Dot(v3(planeX), camPos) - planeX.w) / -Dot(v3(planeX), mouseRayDir);
					float yTrace = (Dot(v3(planeY), camPos) - planeY.w) / -Dot(v3(planeY), mouseRayDir);
					float zTrace = (Dot(v3(planeZ), camPos) - planeZ.w) / -Dot(v3(planeZ), mouseRayDir);
                    
					float minTrace = Min(Min(xTrace, yTrace), zTrace);
					Vec3 rayHitX = Add(camPos, Mul(mouseRayDir, v3(xTrace)));
					Vec3 rayHitY = Add(camPos, Mul(mouseRayDir, v3(yTrace)));
					Vec3 rayHitZ = Add(camPos, Mul(mouseRayDir, v3(zTrace)));
                    
                    
                    
					local_persist Mat4 prevRotation = CreateMatrix(1);
                    
					if (gui->mouseDown)
					{
						if (xTrace < 0) xTrace = Max(yTrace, zTrace);
						if (yTrace < 0) yTrace = Max(xTrace, zTrace);
						if (zTrace < 0) zTrace = Max(xTrace, yTrace);
                        
						bool hitX = (Abs(Length(Sub(rayHitX, o)) - ringRadius) < ringRange);
						bool hitY = (Abs(Length(Sub(rayHitY, o)) - ringRadius) < ringRange);
						bool hitZ = (Abs(Length(Sub(rayHitZ, o)) - ringRadius) < ringRange);
                        
						if ((hitX || hitZ) && xTrace < yTrace)
						{
							if (hitX && xTrace < zTrace)
								gizmoHands = Gizmo_X;
							else if (hitZ)
								gizmoHands = Gizmo_Z;
						}
						else if ((hitY || hitZ))
						{
							if (hitY && yTrace < zTrace)
								gizmoHands = Gizmo_Y;
							else if (hitZ)
								gizmoHands = Gizmo_Z; 
						}
                        
						if (gizmoHands == Gizmo_X) initialLoc = rayHitX;
						if (gizmoHands == Gizmo_Y) initialLoc = rayHitY;
						if (gizmoHands == Gizmo_Z) initialLoc = rayHitZ;
                        
						prevRotation = CreateMatrix(1);
					}
					if (gui->mouseUp)
						gizmoHands = 0;
                    
                    
					if (gizmoHands)
					{
						float specificPlaneRayTraceDist = 0;
						if (gizmoHands == Gizmo_X) specificPlaneRayTraceDist = xTrace;
						if (gizmoHands == Gizmo_Y) specificPlaneRayTraceDist = yTrace;
						if (gizmoHands == Gizmo_Z) specificPlaneRayTraceDist = zTrace;
                        
						Vec3 specificPlaneNormal;
						if (gizmoHands == Gizmo_X) specificPlaneNormal = xDir;
						if (gizmoHands == Gizmo_Y) specificPlaneNormal = yDir;
						if (gizmoHands == Gizmo_Z) specificPlaneNormal = zDir;
                        
						Vec3 specificPlaneRayHit = Add(camPos, Mul(mouseRayDir, v3(specificPlaneRayTraceDist)));
                        
						Vec3 fromVec = Normalize(Sub(initialLoc, o));
						Vec3 toVec = Normalize(Sub(specificPlaneRayHit, o));
                        
						GizmoArrow(o, Add(o, Mul(fromVec, v3(ringRadius) )), s * 0.01, s * 0.015, s * 0.03, v3(0.5));
						GizmoArrow(o, Add(o, Mul(toVec,   v3(ringRadius) )), s * 0.01, s * 0.015, s * 0.03, v3(0.5));
                        
						float fromToRot = Acos(Dot(fromVec, toVec));
						Vec3 fromToAxis = Cross(fromVec, toVec);
                        
						if (Length(fromToAxis) != 0) {
							Mat4 rotation = RotationMatrixAxisAngle(fromToAxis, fromToRot);
							Mat4 appliedRotation = Mul(rotation, Inverse(prevRotation));
							ApplyRotation(editor.selectedEntities, appliedRotation);
							prevRotation = rotation;
                            
							if (!individualOrigins)
							{
								// rotate the actors around the average point!
                                
                                
								For (editor.selectedEntities)
								{
									Entity* actor = GetWorldEntity(&editor.world, *it);
                                    
									Vec3 relativePosition = actor->xform.position - o;
									relativePosition = Mul(relativePosition, 1, appliedRotation);
									actor->xform.position = relativePosition + o;
								}
                                
							}
						}
					}
                    
					break;
				}
			}
		}
	}
    
    
	{ // terrain tools
		DoTerrainTool(gui);
	}
    
    
    
    
	{ // scene view panel
		// ~Todo: scrolling @@Scrolling
		Panel panel = MakePanel(gui->width - 400, gui->height, 400);
		float maxHeight = gui->height - 500;
		
		float titleHeight = 32;
		PanelRow(&panel, titleHeight);
		Gui_TitleBar(gui, "Scene View", panel.atX, panel.atY, panel.width, titleHeight);
        
        
		
		{
			auto actorNames = MakeDynArray<StringView>(0, Frame_Arena);
			auto EntityIds = MakeDynArray<EntityId>(0, Frame_Arena);
            
			For (world->entityPools)
			{
				for (EntityPoolIt actorIt = EntityPoolBegin(*it); EntityPoolItValid(actorIt); actorIt = EntityPoolNext(actorIt))
				{
					Entity* actor = GetEntityFromPoolIt(actorIt);
					Assert(actor != NULL);
                    
					if (actor->flags & Entity_Runtime) continue; // Don't show runtime entities in the scene view

					EntityRegEntry regEntry;
					if (EntityEntryTypeId(actor->typeId, &regEntry)) {					
						String name = (actor->name.length != 0) ? TPrint("%s (%s)", actor->name.data, regEntry.name) : TCopyString(regEntry.name);

						ArrayAdd(&actorNames, GetStringView(name));
						ArrayAdd(&EntityIds, actor->id);
					}
					else
					{
						LogWarn("[editor] Tried to display invalid actor type in scene view! (actor id = %lu)", actor->id);
					}
				}
			}
            
            
			local_persist float scroll = 0;
			float listHeight = maxHeight - (panel.topY - panel.atY);
            
			DynArray<int> selected = MakeDynArray<int>(0, Frame_Arena);
			For (editor.selectedEntities)
			{
				Size idx = ArrayFind(&EntityIds, *it);
				Assert(idx != -1);
                
				ArrayAdd(&selected, (int)idx);
			}
            
			if (Gui_ListViewMultiselect(&game->gui, panel.atX, panel.atY - listHeight, panel.width, listHeight, actorNames, &selected, &scroll))
			{
				ArrayClear(&editor.selectedEntities);
				For (selected)
					SelectActor(EntityIds[*it], true);
			}
			PanelRow(&panel, listHeight);
		}
        
		CompletePanel(&panel);
	}
    
	{ // actor prop panel
		Panel panel = MakePanel(gui->width - 400, 400, 400);
        
		EntityId EntityId = editor.selectedEntities.size == 1 ? editor.selectedEntities[0] : 0; // ~Refactor @@MultiSelect
        
		StringView actorName = editor.selectedEntities.size > 1 ? "(multi-select)" : "[null]";
        
		Entity* actor = GetWorldEntity(world, EntityId);
		EntityRegEntry regEntry;
        
		if (actor && EntityEntryTypeId(actor->typeId, &regEntry)) {
			actorName = regEntry.name;
		}
        
		// draw title
		float titleHeight = 32;
		PanelRow(&panel, titleHeight);
		Gui_TitleBar(gui, TPrint("%s (%lu)", actorName.data, EntityId), panel.atX, panel.atY, panel.width, titleHeight);
        
        
		float rowHeight = 30;
		if (actor)
		{
            
			float textPad = 5;
            
			// ~Hack ~CleanUp
			{
				PanelRow(&panel, rowHeight);
				Gui_DrawRect(gui, panel.atX, panel.atY, panel.width, rowHeight, v4(0));
				Gui_DrawText(gui, "name: ", panel.atX + textPad, panel.atY + rowHeight / 2, GuiTextAlign_Left, &editor.uiFont, v4(1));
				{
					local_persist String text = MakeString();
					auto interact = Gui_Textbox(gui, &text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
					
					if (interact.enter)
					{
						// Update actor name
						String tempString = TempString();

						for (int i = 0; i < text.length; i++)
						{
							if (text.data[i] != ' ')
							{
								tempString.Concat(CharStr(&text.data[i]));
							}
						}

						text.Update(tempString);
						actor->name.Update(text);
					}
					else if (!interact.active) {
						text.Update(actor->name);
					}
				}

				PanelRow(&panel, rowHeight);
				Gui_DrawRect(gui, panel.atX, panel.atY, panel.width, rowHeight, v4(0));
				Gui_DrawText(gui, "x: ", panel.atX + textPad, panel.atY + rowHeight / 2, GuiTextAlign_Left, &editor.uiFont, v4(1));
				{
					local_persist String text = MakeString();
					auto interact = Gui_Textbox(gui, &text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
					
					if (interact.enter)
					{
						float floatVal;
						if (sscanf(text.data, "%f", &floatVal) == 1)
						{
							actor->xform.position.x = floatVal;
						}
						else 
						{
							LogWarn("[editor] textbox float input was invalid");
						}
					}
					else if (!interact.active) {
						StringView value = TPrint("%.2f", actor->xform.position.x);
						text.Update(value);
					}
				}
                
				PanelRow(&panel, rowHeight);
				Gui_DrawRect(gui, panel.atX, panel.atY, panel.width, rowHeight, v4(0));
				Gui_DrawText(gui, "y: ", panel.atX + textPad, panel.atY + rowHeight / 2, GuiTextAlign_Left, &editor.uiFont, v4(1));
				{
					local_persist String text = MakeString();
					auto interact = Gui_Textbox(gui, &text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
					
					if (interact.enter)
					{
						float floatVal;
						if (sscanf(text.data, "%f", &floatVal) == 1)
						{
							actor->xform.position.y = floatVal;
						}
						else 
						{
							LogWarn("[editor] textbox float input was invalid");
						}
					}
					else if (!interact.active) {
						StringView value = TPrint("%.2f", actor->xform.position.y);
						text.Update(value);
					}
				}
                
				PanelRow(&panel, rowHeight);	
				Gui_DrawRect(gui, panel.atX, panel.atY, panel.width, rowHeight, v4(0));
				Gui_DrawText(gui, "z: ", panel.atX + textPad, panel.atY + rowHeight / 2, GuiTextAlign_Left, &editor.uiFont, v4(1));
				{
					local_persist String text = MakeString();
					auto interact = Gui_Textbox(gui, &text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
					
					if (interact.enter)
					{
						float floatVal;
						if (sscanf(text.data, "%f", &floatVal) == 1)
						{
							actor->xform.position.z = floatVal;
						}
						else 
						{
							LogWarn("[editor] textbox float input was invalid");
						}
					}
					else if (!interact.active) {
						StringView value = TPrint("%.2f", actor->xform.position.z);
						text.Update(value);
					}
				}
			}
            
			// properties menu
			bool invalidate = false;
            
			auto* data = actor->GetDataLayout();
			int dataIdx = 0;
			if (data)
				while (data->type != 0)
            {
                PanelRow(&panel, rowHeight);
                Gui_DrawRect(gui, panel.atX, panel.atY, panel.width, rowHeight, v4(0));
                
                
                Gui_DrawText(gui, data->fieldName, panel.atX + textPad, panel.atY + rowHeight / 2, GuiTextAlign_Left, &editor.uiFont, v4(1));
                
                // ~Todo put this in the tools function
                void* rawValue = (u8*)actor + data->offset;
                StringView value = "[unknown type]";
                switch (data->type)
                {
                    case FIELD_INT: 	value = TPrint("%d", *((s32*)rawValue)); break;
                    case FIELD_FLOAT: 	value = TPrint("%.2f", *((float*)rawValue)); break;
                    case FIELD_STRING: 	value = *((String*)rawValue); break;
                    
                    case FIELD_VECTOR2: value = TPrint("(%.2f,%.2f)", 	  			((Vec2*)rawValue)->x, ((Vec2*)rawValue)->y); break;
                    case FIELD_VECTOR3: value = TPrint("(%.2f,%.2f,%.2f)", 	  		((Vec3*)rawValue)->x, ((Vec3*)rawValue)->y, ((Vec3*)rawValue)->z); break;
                    case FIELD_VECTOR4: value = TPrint("(%.2f,%.2f,%.2f,%.2f)", 	((Vec4*)rawValue)->x, ((Vec4*)rawValue)->y, ((Vec4*)rawValue)->z, ((Vec4*)rawValue)->w); break;
                    
                    case Field_Entity: 	value = TPrint("%lu", *((u64*)rawValue)); break;
                    case Field_Model: {
                        ModelResource* model = *((ModelResource**)rawValue);
                        value = model == NULL ? "[null]" : GetStringView(model->name); break;
                    } 	
                }
                
                Editor_PropertyPanelField* field = &editor.selectData.fields[dataIdx++];
                String* text = &field->textValue;
                
                // ~Fix ~CleanUp
                if (data->type == FIELD_INT)
                {
                    auto interact = Gui_Textbox(gui, text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
                    
                    if (interact.enter)
                    {
                        s32 intVal;
                        if (sscanf(text->data, "%d", &intVal) == 1)
                        {
                            s32* rawValueInt = (s32*)rawValue;
                            *rawValueInt = intVal;
                            
                            
                            invalidate = true;
                        }
                        else
                        {
                            LogWarn("[editor] textbox int input was invalid");
                        }
                        
                    }
                    else if (!interact.active)
                        text->Update(value.data);
                }
                else if (data->type == Field_Entity)
                { // ~Todo: actor select panel
                    auto interact = Gui_Textbox(gui, text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
                    
                    if (interact.enter)
                    {
                        u64 intVal;
                        if (sscanf(text->data, "%lu", &intVal) == 1)
                        {
                            u64* rawValueInt = (u64*)rawValue;
                            *rawValueInt = intVal;
                            
                            
                            invalidate = true;
                        }
                        else 
                        {
                            LogWarn("[editor] textbox int input was invalid");
                        }
                    }
                    else if (!interact.active)
                        text->Update(value.data);
                }
                else if (data->type == FIELD_FLOAT)
                {
                    auto interact = Gui_Textbox(gui, text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
                    
                    if (interact.enter)
                    {
                        float floatVal;
                        if (sscanf(text->data, "%f", &floatVal) == 1)
                        {
                            float* rawValueFloat = (float*)rawValue;
                            *rawValueFloat = floatVal;
                            
                            invalidate = true;
                        }
                        else 
                        {
                            LogWarn("[editor] textbox float input was invalid");
                        }
                    }
                    else if (!interact.active)
                        text->Update(value.data);
                }
                else if (data->type == FIELD_STRING)
                {
                    auto interact = Gui_Textbox(gui, text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
                    
                    if (interact.enter)
                    {
                        String* str = (String*)rawValue;
                        str->Update(*text);
                        
                        
                        invalidate = true;
                    }
                    else if (!interact.active)
                        text->Update(value.data);
                }
                else if (data->type == Field_Model)
                {
                    auto interact = Gui_Textbox(gui, text, 64, panel.atX + panel.width / 2, panel.atY, panel.width / 2, rowHeight);
                    
                    local_persist bool modelBrowser = false;
                    
                    if (Gui_Button(gui, "...", panel.atX + panel.width - rowHeight, panel.atY, rowHeight, rowHeight))
                    {
                        modelBrowser = !modelBrowser;
                    }
                    if (modelBrowser)
                    {
                        String strVal;
                        if (ModelBrowser(gui, &strVal))
                        {
                            modelBrowser = false;
                            *((ModelResource**)rawValue) = LoadModel(strVal);
                            
                            invalidate = true;
                        }
                    }
                    
                    if (interact.enter)
                    {
                        *((ModelResource**)rawValue) = LoadModel(*text); // ~Hack ~Refactor do error checking
                        
                        invalidate = true;
                    }
                    else if (!interact.active)
                        text->Update(value.data);
                }
                else
                {
                    Gui_DrawText(gui, value, panel.atX + panel.width - textPad, panel.atY + rowHeight / 2, GuiTextAlign_Right, &editor.uiFont, v4(1));
                }
                
                data++;
            }
            
			if (invalidate)
			{
				ReloadEntity(world, actor);
			}
            
			
		}
        
		if (editor.selectedEntities.size > 0)
		{
			PanelRow(&panel, rowHeight);
			if (Gui_Button(gui, "Delete", panel.atX, panel.atY, panel.width, rowHeight))
			{
				For (editor.selectedEntities)
				{
					Entity* actor = GetWorldEntity(&editor.world, *it);
					Assert(actor);
					
					DeleteEntity(world, actor);
				}
				// Deselect the actor!
				ArrayClear(&editor.selectedEntities);
			}
		}
        
        
		CompletePanel(&panel);
	}
    
    
    
	local_persist bool showActorSpawner = false;
	if (showActorSpawner) { // actor spawner browser
		float width = 400;
		float height = 400;
		Panel panel = MakePanel(gui->width / 2 - width / 2, gui->height / 2 + height / 2, width);
        
		float titleHeight = 32;
		PanelRow(&panel, titleHeight);
		Gui_TitleBar(gui, "Entity Spawner", panel.atX, panel.atY, panel.width, titleHeight);
		float xButtonPad = 5;
		if (Gui_Button(gui, "X", panel.atX + xButtonPad, panel.atY + xButtonPad, titleHeight - xButtonPad * 2, titleHeight - xButtonPad * 2))
		{
			showActorSpawner = false;
		}
        
        
        
        
		local_persist String query = MakeString();
		PanelRow(&panel, 32);
		Gui_Textbox(gui, &query, 64, panel.atX, panel.atY, panel.width, 32);
        
        
		Size queryLen = query.length;
        
		float rowHeight = 25;
		auto registrar = GetEntityRegistrar();
		auto searchResults = MakeDynArray<StringView>(0, Frame_Arena);
		auto searchResultIds = MakeDynArray<EntityTypeId>(0, Frame_Arena);
		For (registrar)
		{
			if (MatchSearchResult(it->name, query.data)) continue; // ~Refactor ~CleanUp string custom util replace string.h
            
			ArrayAdd(&searchResults, StringView(it->name));
			ArrayAdd(&searchResultIds, it->typeId);
		}
        
		float listHeight = 300;
		local_persist float scroll = 0;
		local_persist int selectedIndex = 0;
        
		const float Actor_Spawn_Distance_From_Camera = 5;
        
		if (Gui_ListView(gui, panel.atX, panel.atY - listHeight, panel.width, listHeight, searchResults, &selectedIndex, &scroll))
		{
			Xform xform = CreateXform();
			xform.position = editor.camera.xform.position + OrientForward(XformMatrix(editor.camera.xform)) * v3(Actor_Spawn_Distance_From_Camera);
			Entity* actor = SpawnEntity(searchResultIds[selectedIndex], world, xform);
            
			showActorSpawner = false;
			SelectActor(actor->id, false); // @@MultiSelect ~Refactor
            
			scroll = 0;
			selectedIndex = 0;
		}
		PanelRow(&panel, listHeight);
		CompletePanel(&panel);
	}
    
    
	// menu
	{
		Panel panel = MakePanel(44, gui->height - 44, 128);
		
		float rowHeight = 32;
        
		PanelRow(&panel, rowHeight);
		if (Gui_Button(gui, "New", panel.atX, panel.atY, panel.width, rowHeight))
		{
			ArrayClear(&editor.selectedEntities);
			*world = CreateWorld();
		}
        
		PanelRow(&panel, rowHeight);
		if (Gui_Button(gui, "Open", panel.atX, panel.atY, panel.width, rowHeight))
		{
			ArrayClear(&editor.selectedEntities);
            
			DeleteWorld(world);
			if (LoadWorld("map/world", world))
			{
				For (world->sectors.keys)
					LoadSector(world, *it);
			}
			else
			{
				LogWarn("[editor] Failed to open world!");				
			}
		}
        
		PanelRow(&panel, rowHeight);
		if (Gui_Button(gui, "Save", panel.atX, panel.atY, panel.width, rowHeight))
		{
			// ~Hack
			if (SaveWorld("map/world", world))
			{
			}
			else 
			{
				LogWarn("[editor] Failed to save world!");
			}
		}
        
		// ~Temp
		PanelRow(&panel, rowHeight);
		if (Gui_Button(gui, "Spawn", panel.atX, panel.atY, panel.width, rowHeight))
		{
			showActorSpawner = true;
		}
	}
    
    
    // handling keybinds
    {
        if (editor.input.keyHeld[K_LCTRL] && editor.input.keyDown['c'])
        {
            
            // empty copy buffer
            For (editor.copyBuffer.actors)
                FreeArray(it);
            ArrayClear(&editor.copyBuffer.actors);
            
            For (editor.selectedEntities)
            {
                EntityId id = *it;
                
                Entity* actorToCopy = GetWorldEntity(&editor.world, id);
                if (!actorToCopy)
                {
                    LogWarn("[editor] Tried to copy a NULL actor!");
                    continue;
                }
                
                Array<u8> tempCopyData;
                if (!SerializeEntity(actorToCopy, &tempCopyData))
                {
                    LogWarn("[editor] Could not serialize actor into copy buffer!");
                    continue;
                }
                
                auto copyData = MakeArray<u8>(tempCopyData.size);
                ArrayCopy(copyData, tempCopyData);
                
                ArrayAdd(&editor.copyBuffer.actors, copyData);
            }
            
            Log("[editor] copied %lu actors!", editor.copyBuffer.actors.size);
        }
        
        if (editor.input.keyHeld[K_LCTRL] && editor.input.keyDown['v'])
        {
            if (!editor.copyBuffer.actors.size)
                Log("[editor] copy buffer is empty!");
            
            For (editor.copyBuffer.actors)
            {
                Entity* entity = DeserializeEntity(*it, &editor.world);
                entity->id = GenEntityId(); // We need to give the entity a new id so it isn't just the old one
                // ~Refactor(voidless) move the id out of the entity file again, because the id is gonna be new every time here!
                // makes things simpler!
            }
            
            Log("[editor] pasted %lu actors!", editor.copyBuffer.actors.size);
        }



		if (editor.input.keyHeld[K_LCTRL] && editor.input.keyDown['s'])
        {
			// ~Hack
			if (!SaveWorld("map/world", world))
			{
				LogWarn("[editor] Failed to save world!");
			}
			else 
			{
				SaveTerrain(world); // @@TerrainWorlds ~Refactor move this into world
			}
		}
    }
    
    
    
	{ // reset input
        for (s32 i = 0; i < MB_Count; i++)
        {
            editor.input.mbDown[i] = false;
            editor.input.mbUp[i]   = false;
        }
        
        for (s32 i = 0; i < Keycode_Count; i++)
        {
            editor.input.keyDown[i] = false;
            editor.input.keyUp[i]   = false;
        }
	}
}

void EditorRender(RenderSystem* renderSystem, RenderInfo renderInfo)
{
	RendererHandle swapchainFbo = renderSystem->renderTargets[renderInfo.swapchainImageIndex];
	
	CameraConstants cameraConstants;
	cameraConstants.projectionMatrix = editor.camera.projectionMatrix;
	cameraConstants.viewMatrix = Inverse(XformMatrix(editor.camera.xform));
    
	// ~Todo HEAVILY refactor the rendering systems for this
	// make the game and the editor use the same render infrastructure
	// however allow multiple render paths for easier visibility in the editor?
    
	RenderView(renderSystem, swapchainFbo, cameraConstants, &editor.world);
}


///
bool ModelBrowser(Gui* gui, String* model)
{
	float width = 700;
	Panel panel = MakePanel((gui->width - width) / 2, gui->height / 2 + 300, width);
    
	float rowHeight = 20;
    
	PanelRow(&panel, 32);
	Gui_TitleBar(gui, "Model Browser", panel.atX, panel.atY, panel.width, 32);
    
	PanelRow(&panel, rowHeight);
	local_persist String text = MakeString();
	auto interact = Gui_Textbox(gui, &text, 64, panel.atX, panel.atY, panel.width, rowHeight);
    
	auto modelNames = AllModelNames();
	auto searchResults = MakeDynArray<StringView>(0, Frame_Arena);
	For (modelNames)
	{
		if (MatchSearchResult(it->data, text.data)) continue;
        
		ArrayAdd(&searchResults, GetStringView(*it));		
	}
    
	float listHeight = 300;
	local_persist float scroll = 0;
	local_persist int selectedIndex = 0;
	if (Gui_ListView(gui, panel.atX, panel.atY - listHeight, panel.width, listHeight, searchResults, &selectedIndex, &scroll))
	{
		*model = TCopyString(searchResults[selectedIndex]);
        
		scroll = 0;
		selectedIndex = 0;
        
		return true;	
	}
	PanelRow(&panel, listHeight);
    
	CompletePanel(&panel);
    
	return false;
}