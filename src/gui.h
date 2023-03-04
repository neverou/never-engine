#pragma once

#include "renderer.h"

#include "std.h"
#include "maths.h"

typedef u64 GuiId;
constexpr u64 GuiId_None = -1;

struct GuiVertex
{
	Vec3 position;
	Vec3 color;
	Vec2 uv;
};

struct GuiConstants
{
	Mat4 projectionMatrix;
};

struct GuiWidgetHitbox
{
	GuiId widgetId;
	float x, y, width, height;
};

struct GuiImage
{
	RendererHandle texture;
	u32 x, y, width, height;
};

struct GuiNineSlice
{
	GuiImage image;
	u32 left, right, top, bottom;
};

enum GuiStylePropType
{
	GuiStyleProp_Color = 0,
	GuiStyleProp_Image,
	GuiStyleProp_NineSlice,
};

struct GuiStyleProp
{
	GuiStylePropType type;
	union
	{
		Vec4 color;
		GuiImage image;
		GuiNineSlice nineSlice;
	};
};


#include "stb/stb_truetype.h"

struct GuiFont 
{
	u8* data;
	stbtt_bakedchar chardata[128];

	RendererHandle atlas;
};



struct GuiDrawVertTexturePair
{
	RendererHandle 		texture;
	DynArray<GuiVertex> vertices;
};


enum GuiTextAlign
{
	GuiTextAlign_Center = 0x0,
	GuiTextAlign_Left 	= 0x1,
	GuiTextAlign_Right 	= 0x2,
	GuiTextAlign_Up 	= 0x4,
	GuiTextAlign_Down 	= 0x8,
};

//


struct GuiButtonStyle
{
	float paddingLeft;
	float paddingRight;
	float paddingTop;
	float paddingBottom;

	GuiStyleProp normalBg;
	GuiStyleProp hoverBg;
	GuiStyleProp pressedBg;

	Vec4 normalFgColor;
	Vec4 hoverFgColor;
	Vec4 pressedFgColor;

	GuiFont* font;
};

struct GuiTitleBarStyle
{
	float paddingLeft;
	float paddingRight;
	float paddingTop;
	float paddingBottom;

	GuiStyleProp bg;
	Vec4 fgColor;

	GuiFont* font;
};

struct GuiTextboxStyle
{
	float paddingLeft;
	float paddingRight;
	float paddingTop;
	float paddingBottom;

	GuiStyleProp bg;
	Vec4 fgColor;

	GuiFont* font;
};


struct GuiListViewStyle
{
	float entryHeight;


	float entryPadding;

	GuiStyleProp entryBg;
	GuiStyleProp entryHoverBg;
	GuiStyleProp entryPressedBg;
	GuiStyleProp entrySelectedBg;

	Vec4 fgColor;
	Vec4 hoverFgColor;
	Vec4 pressedFgColor;
	Vec4 selectedFgColor;

	GuiStyleProp bg;

	GuiFont* font;
};

struct GuiSliderStyle
{
	GuiStyleProp sliderBar;
	GuiStyleProp sliderDot;
};

struct GuiStyle
{
	GuiButtonStyle 		button;
	GuiTitleBarStyle 	titleBar;
	GuiTextboxStyle 	textbox;
	GuiListViewStyle 	listView;
	GuiSliderStyle		slider;
};



struct Gui
{
	Renderer* renderer;

	DynArray<GuiDrawVertTexturePair> drawVertPairs;
	RendererHandle drawBuffer;

	RendererHandle guiConstantBuffer;
	GuiConstants guiConstants;
	RendererHandle shader;

	u32 width, height;

	// state
	GuiId hotId;
	GuiId activeId;

	// input
	float mouseX;
	float mouseY;
	float scrollDelta;

	bool mouseDown;
	bool mouseHeld;
	bool mouseUp;

	bool lshift;


	// text input
	char inputBufferComposition[32];
	int cursorPos;


	//
	u8 keyInputBuffer[1024];
	u32 keyInputHead;



////////////////////////////////////
	// Old GUI!:
	//

	// Style
	GuiStyle style;

	// widget stack
	DynArray<GuiWidgetHitbox> widgetHitboxes;

	// z-stack
	u32 z;
};

constexpr u32 Gui_MaxZ = 10000;


bool InitGui(Gui* gui, Renderer* renderer, u32 width, u32 height);
void DestroyGui(Gui* gui);

void BeginGui(Gui* gui);
void EndGui(Gui* gui);

void RenderGui(Gui* gui);
void ResizeGui(Gui* gui, u32 width, u32 height);

void Gui_SetMouseButtonState(Gui* gui, bool held);

//
bool LoadGuiFont(Gui* gui, StringView path, GuiFont* font);
void FreeGuiFont(Gui* gui, GuiFont* font);








/////////////////////////
// Beware,
// Old UI!!!
// 
// NOTE(...): Dont use this, I'm gonna remove it soon!
/////////////////////////


#define Gui_Hash(x, y, w, h) (__COUNTER__ + (u32)x * 14124 + 45178294 * (u32)y)

// widgets
bool _Gui_Button(Gui* gui, StringView text, float x, float y, float width, float height, GuiId id);
#define Gui_Button(gui, text, x, y, width, height) _Gui_Button(gui, text, x, y, width, height, Gui_Hash(x, y, width, height))

bool _Gui_ButtonStyled(Gui* gui, StringView text, float x, float y, float width, float height, GuiButtonStyle* style, GuiId id);
#define Gui_ButtonStyled(gui, text, x, y, width, height, style) _Gui_ButtonStyled(gui, text, x, y, width, height, style, Gui_Hash(x, y, width, height))

void _Gui_TitleBar(Gui* gui, StringView title, float x, float y, float width, float height, GuiId id);
#define Gui_TitleBar(gui, title, x, y, width, height) _Gui_TitleBar(gui, title, x, y, width, height, Gui_Hash(x, y, width, height))

void _Gui_TitleBarStyled(Gui* gui, StringView title, float x, float y, float width, float height, GuiTitleBarStyle* style, GuiId id);
#define Gui_TitleBarStyled(gui, title, x, y, width, height, style) _Gui_TitleBar(gui, title, x, y, width, height, style, Gui_Hash(x, y, width, height))


struct GuiTextboxInteractData
{
	bool active; 	// textbox is selected
	bool changed; 	// textbox value has been changed (when user deselects or presses enter)
	bool changing; 	// textbox value was just changed (user inputs character)
	bool enter; 	// user pressed enter
};

GuiTextboxInteractData _Gui_Textbox(Gui* gui, String* text, u32 maxLength, float x, float y, float width, float height, GuiId id);
#define Gui_Textbox(gui, text, maxLength, x, y, width, height) _Gui_Textbox(gui, text, maxLength, x, y, width, height, Gui_Hash(x, y, width, height)); 

GuiTextboxInteractData _Gui_TextboxStyled(Gui* gui, String* text, u32 maxLength, float x, float y, float width, float height, GuiTextboxStyle* style, GuiId id);
#define Gui_TextboxStyled(gui, text, maxLength, x, y, width, height, style) _Gui_TextboxStyled(gui, text, maxLength, x, y, width, height, style, Gui_Hash(x, y, width, height)); 

bool _Gui_ListViewMultiselect(Gui* gui, float x, float y, float width, float height, Array<StringView> items, DynArray<int>* selectedItems, float* scroll, GuiId id);
#define Gui_ListViewMultiselect(gui, x, y, width, height, items, selectedItems, scroll) _Gui_ListViewMultiselect(gui, x, y, width, height, items, selectedItems, scroll, Gui_Hash(x, y, width, height))

bool _Gui_ListViewMultiselectStyled(Gui* gui, float x, float y, float width, float height, Array<StringView> items, DynArray<int>* selectedItems, float* scroll, GuiListViewStyle* style, GuiId id);
#define Gui_ListViewMultiselectStyled(gui, x, y, width, height, items, selectedItems, scroll, style) _Gui_ListViewMultiselectStyled(gui, x, y, width, height, items, selectedItems, scroll, style, Gui_Hash(x, y, width, height))


bool _Gui_ListView(Gui* gui, float x, float y, float width, float height, Array<StringView> items, int* selectedIndex, float* scroll, GuiId id);
#define Gui_ListView(gui, x, y, width, height, items, selectedIndex, scroll) _Gui_ListView(gui, x, y, width, height, items, selectedIndex, scroll, Gui_Hash(x, y, width, height))

bool _Gui_ListViewStyled(Gui* gui, float x, float y, float width, float height, Array<StringView> items, int* selectedIndex, float* scroll, GuiListViewStyle* style, GuiId id);
#define Gui_ListViewStyled(gui, x, y, width, height, items, selectedIndex, scroll, style) _Gui_ListViewStyled(gui, x, y, width, height, items, selectedIndex, scroll, style, Gui_Hash(x, y, width, height))




bool _Gui_SliderFloat(Gui* gui, float x, float y, float width, float height, float* value, float min, float max, GuiId id);
#define Gui_SliderFloat(gui, x, y, width, height, value, min, max) _Gui_SliderFloat(gui, x, y, width, height, value, min, max, Gui_Hash(x, y, width, height))

bool _Gui_SliderFloatStyled(Gui* gui, float x, float y, float width, float height, float* value, float min, float max, GuiSliderStyle* style, GuiId id);
#define Gui_SliderFloatStyled(gui, x, y, width, height, value, min, max) _Gui_SliderFloatStyled(gui, x, y, width, height, value, min, max, style Gui_Hash(x, y, width, height))




// draw
void Gui_DrawImageUvs(Gui* gui, float x, float y, float width, float height, Vec2 uvs[4], GuiImage image, Vec4 color);
void Gui_DrawImage(Gui* gui, float x, float y, float width, float height, GuiImage image, Vec4 color);
void Gui_DrawRect(Gui* gui, float x, float y, float width, float height, Vec4 color);
void Gui_DrawText(Gui* gui, StringView text, float x, float y, u32 align, GuiFont* font, Vec4 textColor);
void Gui_DrawNineSlice(Gui* gui, float x, float y, float width, float height, float left, float right, float bottom, float top, GuiNineSlice image, Vec4 color);

// util
Vec2 Gui_MeasureText(StringView text, GuiFont* font);

// widget backends
void Gui_DoWidgetHitbox(Gui* gui, float x, float y, float width, float height, GuiId id);


struct Panel
{
	float width;
	float height;

	float leftX;
	float topY;

	float atX;
	float atY;
};


Panel MakePanel(float atX, float atY, float width);
void PanelRow(Panel* panel, float rowHeight);

void CompletePanel(Panel* panel);
