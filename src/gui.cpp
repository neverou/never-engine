#include "gui.h"

#include "mesh.h"
#include "resource.h"
#include "allocators.h"
#include "event.h"

// ~Temp
const u64 Gui_Max_Vertices = 16000;

bool InitGui(Gui* gui, Renderer* renderer, u32 width, u32 height)
{
	gui->renderer = renderer;

	gui->drawVertPairs = MakeDynArray<GuiDrawVertTexturePair>(0, NULL);
	gui->drawBuffer = renderer->CreateBuffer(NULL, sizeof(GuiVertex) * Gui_Max_Vertices, RENDERBUFFER_FLAGS_VERTEX | RENDERBUFFER_FLAGS_USAGE_STREAM);
	if (!gui->drawBuffer)
		LogWarn("[gui] failed to allocate gui buffer! gui will not render");


	// Shader
	auto shaderRes = LoadShader("shaders/gui.shader");
	gui->shader = shaderRes ? shaderRes->handle : 0;

	if (!gui->shader)
		LogWarn("[gui] missing shader! gui will not render.");


	// Gui Constants
	gui->guiConstantBuffer = renderer->CreateBuffer(NULL, sizeof(GuiConstants), RENDERBUFFER_FLAGS_CONSTANT | RENDERBUFFER_FLAGS_USAGE_DYNAMIC);

	ResizeGui(gui, width, height);



	// hitboxes
	gui->widgetHitboxes = MakeDynArray<GuiWidgetHitbox>(0, NULL);

	//

	gui->hotId = gui->activeId = GuiId_None;



	gui->keyInputHead = 0;

	// 

	return true;
}

void DestroyGui(Gui* gui)
{
	FreeDynArray(&gui->drawVertPairs);
	gui->renderer->FreeBuffer(gui->drawBuffer);
	
	gui->renderer->FreeBuffer(gui->guiConstantBuffer);

	FreeDynArray(&gui->widgetHitboxes);
}


#include "game.h" // ~Todo remove this

void RenderGui(Gui* gui)
{
	auto renderer = gui->renderer;

	auto renderSystem = &game->renderSystem; // ~Refactor replace the reference to the renderer with this

	if (gui->shader)
		renderer->CmdSetShader(gui->shader);
	else
		return;




	auto drawVertTmpBuffer = MakeDynArray<GuiVertex>(0, Frame_Arena);
	defer(FreeDynArray(&drawVertTmpBuffer));

	auto textureIndices = MakeDynArray<RendererHandle>(0, Frame_Arena);
	auto textureOffsets = MakeDynArray<u32>(0, Frame_Arena);

	For (gui->drawVertPairs)
	{
		ArrayAdd(&textureOffsets, (u32)drawVertTmpBuffer.size);
		ArrayAdd(&textureIndices, it->texture);

		ForIt (it->vertices, vertIt)
		{
			ArrayAdd(&drawVertTmpBuffer, *vertIt);
		}
	}

	// ~Temp ~Incomplete ~Refactor add reallocate
	Assert(drawVertTmpBuffer.size <= Gui_Max_Vertices);

	void* data = renderer->MapBuffer(gui->drawBuffer);
	Memcpy(data, drawVertTmpBuffer.data, drawVertTmpBuffer.size * sizeof(GuiVertex));
	renderer->UnmapBuffer(gui->drawBuffer);


	Assert(textureIndices.size == textureOffsets.size);

	constexpr s32 GuiMode_Color = 0;
	constexpr s32 GuiMode_Image = 1;

	for (u32 i = 0; i < textureIndices.size; i++)
	{
		RendererHandle texture = textureIndices[i];
		u32 textureOffset = textureOffsets[i];
		u32 nextTextureOffset = (i + 1 >= textureOffsets.size) ? (drawVertTmpBuffer.size) : (textureOffsets[i + 1]);

		FixArray<RendererHandle, 1> consts;
		consts[0] = gui->guiConstantBuffer;
		renderer->CmdSetConstantBuffers(gui->shader, 0, &consts);

		s32 guiMode = (texture == 0) ? GuiMode_Color : GuiMode_Image;
		if (guiMode == GuiMode_Image)
		{
			FixArray<TextureSampler, 1> tex;
			tex[0].textureHandle = texture;
			tex[0].samplerHandle = ProcureSuitableSampler(renderSystem->renderer, Filter_Linear, Filter_Linear, SamplerAddress_Repeat, SamplerAddress_Repeat, SamplerAddress_Repeat, SamplerMipmap_Linear);
			renderer->CmdSetSamplers(gui->shader, 15, &tex);
		}

		renderer->CmdPushConstants(gui->shader, 0, sizeof(s32), &guiMode);

		FixArray<RendererHandle, 1> vb;
		vb[0] = gui->drawBuffer;
		renderer->CmdBindVertexBuffers(0, &vb, NULL);
		renderer->CmdDrawIndexed(nextTextureOffset - textureOffset, 0, textureOffset, 1, 0); // ~FixMe use CmdDraw instead of CmdDrawIndexed
	}
}

void BeginGui(Gui* gui)
{
	ArrayClear(&gui->drawVertPairs);
	ArrayClear(&gui->widgetHitboxes);

	gui->z = Gui_MaxZ;
}

void EndGui(Gui* gui)
{
	gui->hotId = GuiId_None;

	// set the hot element
	For (gui->widgetHitboxes)
	{
		if ((it->x)             <= gui->mouseX	&& (it->y) <= gui->mouseY           && 
			(it->x + it->width) > gui->mouseX 	&& (it->y + it->height) > gui->mouseY)
		{
			gui->hotId = it->widgetId;

			// Dont break from the loop therefore the last element will be the selected
			// oh wait ~Cleanup just make the loop go backwards and then break when u find a hit lol
		}
	}

	if (gui->mouseDown && gui->hotId == GuiId_None) gui->activeId = GuiId_None;


	// reset input
	gui->mouseUp = gui->mouseDown = false;
	gui->keyInputHead = 0;
	gui->scrollDelta = 0;
}

void ResizeGui(Gui* gui, u32 width, u32 height)
{
	Log("[gui] resizing to (%ux%u)", width, height);
	gui->width = width;
	gui->height = height;


	gui->guiConstants.projectionMatrix = OrthoMatrix(0, gui->width, 0, gui->height, 0, Gui_MaxZ);

	// Update the constants
	void* constants = gui->renderer->MapBuffer(gui->guiConstantBuffer);
	Memcpy(constants, &gui->guiConstants, sizeof(GuiConstants));
	gui->renderer->UnmapBuffer(gui->guiConstantBuffer);
}

void Gui_SetMouseButtonState(Gui* gui, bool held)
{
	if  (held) gui->mouseDown = true;
	if (!held) gui->mouseUp   = true;
	gui->mouseHeld = held;
}


//

#include "sys.h"
#include "util.h"

bool LoadGuiFont(Gui* gui, StringView path, GuiFont* font)
{
	Assert(gui != NULL);
	Assert(font != NULL);

	File file;
	if (!Open(&file, path, FILE_READ))
	{
		LogWarn("[gui] Failed to read font file! (%s)", path.data);
		return false;
	}
	defer(Close(&file));

	Size size = FileLength(&file);

	font->data = (u8*)Alloc(size);
	Read(&file, font->data, size);	

	int fontCount = stbtt_GetNumberOfFonts(font->data);
	if (fontCount != 1)
		LogWarn("[gui] font file has %d ( > 1) fonts %s", fontCount, path.data);


	// // this doesnt work if the .tff file only has one font but thats fineeeeee
	// if (!stbtt_InitFont(&stbFont, font->data, 0)) 
	// {
	// 	LogWarn("[gui] loading font failed:  (%s)", path.data);
	// 	return false;
	// }

	
	u8* pixels = (u8*)TempAlloc(512 * 512);
	if (!stbtt_BakeFontBitmap(font->data, 0, 
							  24,
							  pixels, 512, 512,
							  0, 128,
							  font->chardata))
	{
		LogWarn("[gui] baking font failed! (%s)", path.data);
		return false;
	}

	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			u8 a = pixels[x + y * 512];
			pixels[x + y * 512] = pixels[x + (511 - y) * 512];
			pixels[x + (511 - y) * 512] = a;
		}
	}

	u32* pixelsRgba = (u32*)TempAlloc(512 * 512);
	for (int i = 0; i < 512 * 512; i++)
	{
		pixelsRgba[i] = pixels[i] | (pixels[i] << 8) | (pixels[i] << 16) | (pixels[i] << 24);
	}

	font->atlas = gui->renderer->CreateTexture(512, 512, FORMAT_R8G8B8A8_UNORM, 1, TextureFlags_Sampled);
	gui->renderer->UploadTextureData(font->atlas, 512 * 512 * 4, pixelsRgba);

	return true;
}

void FreeGuiFont(Gui* gui, GuiFont* font)
{
	Assert(gui != NULL);
	Assert(font != NULL);

	Free(font->data);
}

//

intern void Gui_PushVertex(Gui* gui, GuiVertex vertex, RendererHandle texture)
{
	GuiDrawVertTexturePair* drawVertPair = NULL;

	For (gui->drawVertPairs)
	{
		if (it->texture == texture)
		{
			drawVertPair = it;
			break;
		}
	}

	if (!drawVertPair)
	{
		GuiDrawVertTexturePair pair;
		pair.texture = texture;
		pair.vertices 	 = MakeDynArray<GuiVertex>(0, Frame_Arena);

		drawVertPair = ArrayAdd(&gui->drawVertPairs, pair);
	}

	ArrayAdd(&drawVertPair->vertices, vertex);
}

intern void DrawStyleProp(Gui* gui, float x, float y, float width, float height, GuiStyleProp prop)
{
	if (prop.type == GuiStyleProp_Color)
		Gui_DrawRect(gui, x, y, width, height, prop.color);		
	else if (prop.type == GuiStyleProp_Image)
		Gui_DrawImage(gui, x, y, width, height, prop.image, v4(1));
	else if (prop.type == GuiStyleProp_NineSlice)
		Gui_DrawNineSlice(gui, x, y, width, height, prop.nineSlice.left, prop.nineSlice.right, prop.nineSlice.bottom, prop.nineSlice.top, prop.nineSlice, v4(1));
}


bool _Gui_Button(Gui* gui, StringView text, float x, float y, float width, float height, GuiId id)
{
	return _Gui_ButtonStyled(gui, text, x, y, width, height, &gui->style.button, id);
}


bool _Gui_ButtonStyled(Gui* gui, StringView text, float x, float y, float width, float height, GuiButtonStyle* style, GuiId id)
{	
	bool hot = id == gui->hotId;
	bool active = id == gui->activeId;

	// draw
	GuiStyleProp background = style->normalBg;
	if (hot) 	background = style->hoverBg;
	if (active) background = style->pressedBg;

	DrawStyleProp(gui, x, y, width, height, background);

	Vec4 textColor = style->normalFgColor;
	if (hot) 	textColor = style->hoverFgColor;
	if (active) textColor = style->pressedFgColor;

	// ~Todo button style should have text align (& padding)
	Gui_DrawText(gui, text, x + width / 2, y + height / 2, GuiTextAlign_Center, style->font, textColor);

	// input

	Gui_DoWidgetHitbox(gui, x, y, width, height, id);

	if (active)
	{
		if (hot && gui->mouseUp) {
			gui->activeId = GuiId_None;
			return true;
		}
	} 
	else if (hot)
		if (gui->mouseDown)
			gui->activeId = id;

	return false;
}





void _Gui_TitleBar(Gui* gui, StringView title, float x, float y, float width, float height, GuiId id)
{
	_Gui_TitleBarStyled(gui, title, x, y, width, height, &gui->style.titleBar, id);
}

void _Gui_TitleBarStyled(Gui* gui, StringView title, float x, float y, float width, float height, GuiTitleBarStyle* style, GuiId id)
{
	auto background = style->bg;
	DrawStyleProp(gui, x, y, width, height, background);

	// ~Todo padding & align
	Gui_DrawText(gui, title, x + width / 2, y + height / 2, GuiTextAlign_Center, style->font, v4(0));
}



GuiTextboxInteractData _Gui_Textbox(Gui* gui, String* text, u32 maxLength, float x, float y, float width, float height, GuiId id)
{
	return _Gui_TextboxStyled(gui, text, maxLength, x, y, width, height, &gui->style.textbox, id);
}


#include <string.h> // ~CleanUp (custom string utils)

GuiTextboxInteractData _Gui_TextboxStyled(Gui* gui, String* text, u32 maxLength, float x, float y, float width, float height, GuiTextboxStyle* style, GuiId id)
{
	auto background = style->bg;
	DrawStyleProp(gui, x, y, width, height, background);

	// ~Todo padding & align

	String show = TCopyString(*text);

	if (gui->activeId == id) show.Insert(gui->cursorPos, "|"); // ~Refactor fix this cursor to not literally be a pipe added to the string lol
	Gui_DrawText(gui, show, x + style->paddingLeft, y + height / 2, GuiTextAlign_Left, style->font, v4(0));

	// input
	Gui_DoWidgetHitbox(gui, x, y, width, height, id);

	bool enter = false;


	if (gui->hotId == id)
		if (gui->mouseDown) {
			gui->activeId = id;
			gui->cursorPos = text->length;
		}

	if (gui->activeId == id)
	{
		for (u32 i = 0; i < gui->keyInputHead; i++) {
			u32 keycode = gui->keyInputBuffer[i];
			if (keycode >= 32 && keycode <= 126)
			{
				if (maxLength > 0)
				{
					if (gui->cursorPos + 1 == maxLength) break;
					Assert(gui->cursorPos + 1 < maxLength);
				}

				char key = (char)keycode;
				text->Insert(gui->cursorPos, CharStr(&key));
				gui->cursorPos = text->length;
			}
			else if (keycode == K_BACKSPACE) // backspace
			{
				if (gui->cursorPos > 0) {
					text->RemoveAt(gui->cursorPos - 1);
					gui->cursorPos--;
				}
			}
			else if (keycode == K_RETURN)
			{
				enter = true;
				gui->activeId = GuiId_None;
			}
			else if (keycode == K_LEFTARROW)
			{
				if (gui->cursorPos > 1)
					gui->cursorPos--;
			}
			else if (keycode == K_RIGHTARROW)
			{
				if (gui->cursorPos < text->length)
					gui->cursorPos++;
			}
		}
	}


	GuiTextboxInteractData interact;

	interact.active = gui->activeId == id;
	interact.enter  = enter;

	return interact;
}











bool _Gui_ListViewMultiselect(Gui* gui, float x, float y, float width, float height, Array<StringView> items, DynArray<int>* selectedItems, float* scroll, GuiId id)
{
	return _Gui_ListViewMultiselectStyled(gui, x, y, width, height, items, selectedItems, scroll, &gui->style.listView, id);
}

bool _Gui_ListViewMultiselectStyled(Gui* gui, float x, float y, float width, float height, Array<StringView> items, DynArray<int>* selectedItems, float* scroll, GuiListViewStyle* style, GuiId id)
{
	Gui_DoWidgetHitbox(gui, x, y, width, height, id);
	DrawStyleProp(gui, x, y, width, height, style->bg);
	
	bool changed = false;

	bool anyHot = gui->hotId == id;

	int row = 0;
	For (items)
	{
		float rowY = y + height - style->entryHeight * (row + 1);
		if (scroll) rowY += *scroll;

		// hacky
		if (rowY > y + height - style->entryHeight) { row++; continue; }
		if (rowY < y) { break; }
		
		GuiId entryId = Gui_Hash(x, rowY, width, style->entryHeight);
		Gui_DoWidgetHitbox(gui, x, rowY, width, style->entryHeight, entryId);

		bool active = gui->activeId == entryId;
		bool hot 	= gui->hotId    == entryId;
	
		// input
		if (active)
		{
			if (hot && gui->mouseUp)
			{
				gui->activeId = GuiId_None;

				// pressed
				changed = true;

				if (selectedItems) 
				{
					if (!gui->lshift) // multi select
						ArrayClear(selectedItems); 

					Size idx = ArrayFind(selectedItems, row);
					if (idx == -1)
						ArrayAdd(selectedItems, row);
					else
						ArrayRemoveAt(selectedItems, idx);
				}
			}
		} 
		else if (hot)
			if (gui->mouseDown)
				gui->activeId = entryId;

		anyHot |= hot;


		// ~Refactor: seperate hover styling and selected styling

		GuiStyleProp entryBgStyle = style->entryBg;
		Vec4 fgColor = style->fgColor;

		if (hot) {
			entryBgStyle = style->entryHoverBg;
			fgColor = style->hoverFgColor;
		}  
		if (active) {  
			entryBgStyle = style->entryPressedBg;
			fgColor = style->pressedFgColor;
		}

		bool selected = selectedItems && ArrayFind(selectedItems, row) != -1;
		if (selected)
		{
			entryBgStyle = style->entrySelectedBg;
			fgColor = style->selectedFgColor;
		}

		DrawStyleProp(gui, x, rowY, width, style->entryHeight, entryBgStyle);
		Gui_DrawText(gui, *it, x + style->entryPadding, rowY + style->entryHeight / 2, GuiTextAlign_Left, style->font, fgColor);

		row++;
	}

	if (anyHot && scroll) (*scroll) -= gui->scrollDelta * style->entryHeight;
	if (scroll && *scroll < 0) *scroll = 0;
	float maxScroll = style->entryHeight * items.size - height;
	if (scroll && *scroll > maxScroll) *scroll = maxScroll;


	// Draw a scroll indicator (not usable as a scroll bar atm)
	float scrollPercentage = *scroll / maxScroll;
	float scrollIndicatorWidth = 5; // ~Refactor put this in the styling
	float scrollIndicatorHeight = height / (style->entryHeight * items.size) * height;
	
	if (scrollIndicatorHeight > height) scrollIndicatorHeight = height;

	Gui_DrawRect(gui, x + width - scrollIndicatorWidth, y + height - scrollIndicatorHeight - (height - scrollIndicatorHeight) * scrollPercentage, scrollIndicatorWidth, scrollIndicatorHeight, v4(1,0,0,1));

	return changed;
}

bool _Gui_ListView(Gui* gui, float x, float y, float width, float height, Array<StringView> items, int* selectedIndex, float* scroll, GuiId id)
{
	return _Gui_ListViewStyled(gui, x, y, width, height, items, selectedIndex, scroll, &gui->style.listView, id);
}

bool _Gui_ListViewStyled(Gui* gui, float x, float y, float width, float height, Array<StringView> items, int* selectedIndex, float* scroll, GuiListViewStyle* style, GuiId id)
{
	DynArray<int> selected = MakeDynArray<int>(0, Frame_Arena);
	if (*selectedIndex >= 0)
		ArrayAdd(&selected, *selectedIndex);
	
	bool v = _Gui_ListViewMultiselectStyled(gui, x, y, width, height, items, &selected, scroll, style, id);

	if (selected.size)
		*selectedIndex = selected[selected.size - 1];
	else
		*selectedIndex = -1;

	return v;
}

bool _Gui_SliderFloat(Gui* gui, float x, float y, float width, float height, float* value, float min, float max, GuiId id)
{
	return _Gui_SliderFloatStyled(gui, x, y, width, height, value, min, max, &gui->style.slider, id);
}

bool _Gui_SliderFloatStyled(Gui* gui, float x, float y, float width, float height, float* value, float min, float max, GuiSliderStyle* style, GuiId id)
{
	Gui_DoWidgetHitbox(gui, x, y, width, height, id);

	DrawStyleProp(gui, x, y, width, height, style->sliderBar);

	float percentage = (*value - min) / (max - min);
	float xPosMap = x + (height / 2) + percentage * (width - height);
	DrawStyleProp(gui, xPosMap - (height / 2), y, height, height, style->sliderDot);

	if (gui->hotId == id) {
		if (gui->mouseDown) {
			gui->activeId = id;
		}
	}

	if (gui->activeId == id)
	{
		float mouseUnmap = (gui->mouseX - (x + height / 2)) / (width - height);
		if (mouseUnmap < 0) mouseUnmap = 0;
		if (mouseUnmap > 1) mouseUnmap = 1;
		
		*value = min + (max - min) * mouseUnmap;

		if (gui->mouseUp)
		{
			gui->activeId = GuiId_None;	
			return true;
		}
	}

	return false;
}







//



void Gui_DrawImageUvs(Gui* gui, float x, float y, float width, float height, Vec2 uvs[4], GuiImage image, Vec4 color)
{	
	Vec2 modUvs[4] = { v2(0, 0), v2(0, 1), v2(1, 1), v2(1, 0) };

	auto* texture = gui->renderer->LookupTexture(image.texture);

	float imgX = image.x 		/ float(texture->width - 1);
	float imgY = image.y 		/ float(texture->height - 1);
	float imgW = image.width 	/ float(texture->width - 1);
	float imgH = image.height 	/ float(texture->height - 1);

	// if all image crop dimensions are 0 then just display the whole image
	if (!(image.x == 0 && image.y == 0  && image.width == 0 && image.height == 0))
	{
		for (s32 i = 0; i < 4; i++)
		{
			modUvs[i] = v2(imgX, imgY) + modUvs[i] * v2(imgW, imgH);
		}
	}

	// uv thing
	{
		Vec2 xEdge = modUvs[3] - modUvs[0];
		Vec2 yEdge = modUvs[1] - modUvs[0];

		Vec2 origin = modUvs[0];
		for (s32 i = 0; i < 4; i++)
		{
			modUvs[i] = origin + v2(uvs[i].x) * xEdge + v2(uvs[i].y) * yEdge;
		}
	}

	Assert(image.texture != 0);

	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y, 	 	  gui->z), v3(color), modUvs[0] }, image.texture);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y + height, gui->z), v3(color), modUvs[2] }, image.texture);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y, 	 	  gui->z), v3(color), modUvs[3] }, image.texture);

	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y, 	 	  gui->z), v3(color), modUvs[0] }, image.texture);
	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y + height, gui->z), v3(color), modUvs[1] }, image.texture);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y + height, gui->z), v3(color), modUvs[2] }, image.texture);
	
	gui->z--;
}

void Gui_DrawImage(Gui* gui, float x, float y, float width, float height, GuiImage image, Vec4 color)
{	
	Vec2 uvs[] { v2(0,0), v2(0,1), v2(1,1), v2(1,0) };
	Gui_DrawImageUvs(gui, x, y, width, height, uvs, image, color);
}

void Gui_DrawRect(Gui* gui, float x, float y, float width, float height, Vec4 color)
{
	Vec2 uvs[4] = { v2(0, 0), v2(1, 0), v2(1, 1), v2(0, 1) };

	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y, 	 	  gui->z), v3(color), uvs[0] }, 0);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y + height, gui->z), v3(color), uvs[2] }, 0);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y, 	 	  gui->z), v3(color), uvs[1] }, 0);

	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y, 	 	  gui->z), v3(color), uvs[0] }, 0);
	Gui_PushVertex(gui, GuiVertex { v3(x, 	  	  y + height, gui->z), v3(color), uvs[3] }, 0);
	Gui_PushVertex(gui, GuiVertex { v3(x + width, y + height, gui->z), v3(color), uvs[2] }, 0);

	gui->z--;
}


void Gui_DrawText(Gui* gui, StringView text, float x, float y, u32 textAlign, GuiFont* font, Vec4 textColor)
{
	int alignX = 0;
	int alignY = 0;

	if (textAlign & GuiTextAlign_Left)	alignX++;
	if (textAlign & GuiTextAlign_Right) alignX--;
	if (textAlign & GuiTextAlign_Down) 	alignY++;
	if (textAlign & GuiTextAlign_Up) 	alignY--;

	Vec2 measure = Gui_MeasureText(text, font);

	float xpos = x, ypos = y;
	for (int i = 0; i < text.length; i++) {
		stbtt_aligned_quad quadToDraw;

		char ch = text.data[i];
		stbtt_GetBakedQuad(font->chardata, 512, 512, ch, &xpos, &ypos, &quadToDraw, true);

		GuiImage charImage = {};
		charImage.texture = font->atlas;
		charImage.x 	 = quadToDraw.s0 * 512;
		charImage.y 	 = quadToDraw.t0 * 512;
		charImage.width  = (quadToDraw.s1 - quadToDraw.s0) * 512;
		charImage.height = (quadToDraw.t1 - quadToDraw.t0) * 512;
		charImage.y = 511 - charImage.y - charImage.height;

		float y0src = quadToDraw.y0;
		quadToDraw.y0 = (ypos - quadToDraw.y1) + ypos;
		quadToDraw.y1 = (ypos - y0src) 		   + ypos;

		Vec2 alignOffset;

		alignOffset.x = -measure.x / 2 + alignX * measure.x / 2;
		alignOffset.y = -measure.y / 2 + alignY * measure.y / 2;

		quadToDraw.x0 += alignOffset.x;
		quadToDraw.x1 += alignOffset.x;
		quadToDraw.y0 += alignOffset.y;
		quadToDraw.y1 += alignOffset.y;

		Gui_DrawImage(gui, quadToDraw.x0, quadToDraw.y0, quadToDraw.x1 - quadToDraw.x0, quadToDraw.y1 - quadToDraw.y0, charImage, textColor);
	}
}


void Gui_DrawNineSlice(Gui* gui, float x, float y, float width, float height, float left, float right, float bottom, float top, GuiNineSlice nineSlice, Vec4 color)
{
	u32 imgWidth, imgHeight;
	imgWidth  = nineSlice.image.width;
	imgHeight = nineSlice.image.height;

	Texture* texture = gui->renderer->LookupTexture(nineSlice.image.texture);
	if (imgWidth == 0)
		imgWidth = texture->width;
	if (imgHeight == 0)
		imgHeight = texture->height;

	float uLeft   = nineSlice.left  / float(imgWidth - 1);
	float uRight  = nineSlice.right / float(imgWidth - 1);

	float vTop    = nineSlice.top    / float(imgHeight - 1);
	float vBottom = nineSlice.bottom / float(imgHeight - 1);

	Vec2 uvBL[] {	v2(0,          0), 
					v2(0,          vBottom),
					v2(uLeft,      vBottom), 
					v2(uLeft,      0)    };
	Vec2 uvBR[] {	v2(1 - uRight, 0), 
					v2(1 - uRight, vBottom),
					v2(1,          vBottom), 
					v2(1,          0)    };
	Vec2 uvTL[] {	v2(0,          1 - vTop), 
					v2(0,          1),
					v2(uLeft,      1), 
					v2(uLeft,      1 - vTop)    };
	Vec2 uvTR[] {	v2(1 - uRight, 1 - vTop), 
					v2(1 - uRight, 1),
					v2(1,          1), 
					v2(1,          1 - vTop)    };

	Gui_DrawImageUvs(gui, x,                 y,                left,  bottom, uvBL, nineSlice.image, color); // BL heh
	Gui_DrawImageUvs(gui, x + width - right, y,                right, bottom, uvBR, nineSlice.image, color); // BR
	Gui_DrawImageUvs(gui, x,                 y + height - top, left,  bottom, uvTL, nineSlice.image, color); // TL
	Gui_DrawImageUvs(gui, x + width - right, y + height - top, right, bottom, uvTR, nineSlice.image, color); // TR


	Vec2 uvL[] {
		v2(0,     vBottom),
		v2(0,     1 - vTop),
		v2(uLeft, 1 - vTop),
		v2(uLeft, vBottom),
	};

	Vec2 uvR[] {
		v2(1 - uRight, vBottom),
		v2(1 - uRight, 1 - vTop),
		v2(1,          1 - vTop),
		v2(1,          vBottom),
	};

	Vec2 uvT[] {
		v2(uLeft, 	   1 - vTop),
		v2(uLeft, 	   1),
		v2(1 - uRight, 1),
		v2(1 - uRight, 1 - vTop),
	};

	Vec2 uvB[] {
		v2(uLeft, 	   0),
		v2(uLeft, 	   vBottom),
		v2(1 - uRight, vBottom),
		v2(1 - uRight, 0),
	};

	Gui_DrawImageUvs(gui, x, y + bottom, 				 left,  height - top - bottom, uvL, nineSlice.image, color); // L
	Gui_DrawImageUvs(gui, x + width - right, y + bottom, right, height - top - bottom, uvR, nineSlice.image, color); // R
	Gui_DrawImageUvs(gui, x + left, y + height - top, 	 width - left - right, top,    uvT, nineSlice.image, color); // T
	Gui_DrawImageUvs(gui, x + left, y, 			         width - left - right, bottom, uvB, nineSlice.image, color); // B

	Vec2 uvCenter[] { v2(uLeft,			vBottom), 
					  v2(uLeft,			1 - vTop),
					  v2(1 - uRight,	1 - vTop), 
					  v2(1 - uRight,	vBottom) };
	Gui_DrawImageUvs(gui, x + left, y + bottom, width - right - left, height - top - bottom, uvCenter, nineSlice.image, color); // center
}


//

Vec2 Gui_MeasureText(StringView text, GuiFont* font)
{
	float xpos = 0, ypos = 0;
	Vec2 measure = {};

	for (int i = 0; i < text.length; i++) {
		stbtt_aligned_quad quadToDraw;

		char ch = text.data[i];
		stbtt_GetBakedQuad(font->chardata, 512, 512, ch, &xpos, &ypos, &quadToDraw, true);

		float y0src = quadToDraw.y0;
		quadToDraw.y0 = (ypos - quadToDraw.y1) + ypos;
		quadToDraw.y1 = (ypos - y0src) 		   + ypos;

		measure.x = quadToDraw.x1;
		measure.y = quadToDraw.y1;
	}

	return measure;
}



//




void Gui_DoWidgetHitbox(Gui* gui, float x, float y, float width, float height, GuiId id)
{
	GuiWidgetHitbox hitbox;
	hitbox.widgetId = id;
	hitbox.x 		= x;
	hitbox.y 		= y;
	hitbox.width 	= width;
	hitbox.height 	= height;

	ArrayAdd(&gui->widgetHitboxes, hitbox);
}


//

Panel MakePanel(float atX, float atY, float width)
{
	Panel panel;
	panel.leftX = panel.atX = atX;
	panel.topY  = panel.atY = atY;

	panel.width = width;

	return panel;
}

void PanelRow(Panel* panel, float rowHeight)
{
	panel->atY -= rowHeight;
	panel->atX = panel->leftX;
}


void CompletePanel(Panel* panel)
{
	panel->height = panel->topY - panel->atY;
}