#pragma once

#include "array.h"


enum EventType {
	Event_None,
	Event_Quit,
    
	Event_SurfaceResize,
    
	Event_Keyboard,
	Event_Mouse,
	Event_Gamepad
};

enum KeyboardEventType {
    KEY_EVENT_UP = 0,
	KEY_EVENT_DOWN = 1
};

// ~Rename
enum MouseEventType {
    MOUSE_BUTTON_DOWN,
    MOUSE_BUTTON_UP,
    MOUSE_MOVE,
    Mouse_Scroll,
};

enum GamepadEventType {
    CONTROLLER_BUTTON_DOWN,
    CONTROLLER_BUTTON_UP,
    CONTROLLER_AXIS_MOTION
};

enum Keycode {
    K_NONE = 0,
    
    K_BACKSPACE = 8,
    K_TAB,
    
    K_RETURN = 13,
    
    K_ESCAPE = 27,
    
    
    K_SPACE = 32,
    K_SPAKE = 32, // ~Todo Remove before E3
    K_EXCLAIM,
    K_DOUBLEQUOTE,
    K_HASHTAG,
    K_DOLLAR,
    K_PERCENT,
    K_AMPERSAND,
    K_QUOTE,
    K_LEFTPARENTHESIS,
    K_RIGHTPARENTHESIS,
    K_ASTERISK,
    K_PLUS,
    K_COMMA,
    K_MINUS,
    K_PERIOD,
    K_SLASH,
    K_0,
    K_1,
    K_2,
    K_3,
    K_4,
    K_5,
    K_6,
    K_7,
    K_8,
    K_9,
    K_COLON,
    K_SEMICOLON,
    K_LESSTHAN,
    K_EQUALS,
    K_GREATERTHAN,
    K_QUESTION,
    K_AT,
    
    K_LEFTBRACKET = 91,
    K_BACKSLASH,
    K_RIGHTBRACKET,
    K_CARET,
    K_UNDERSCORE,
    K_GRAVE, // = 96
    
    // lowercase ascii 97(a) - 122(z)
    
    K_LEFTCURLYBRACKET = 123,
    K_VERTICALBAR,
    K_RIGHTCURLYBRACKET,
    
    K_TILDA = 126,
    K_DELETE,
    // all of the above match ascii
    // below are custom codes
    
    K_KP_BACKSPACE,
    
    K_CAPSLOCK,
    K_F1,
    K_F2,
    K_F3,
    K_F4,
    K_F5,
    K_F6,
    K_F7,
    K_F8,
    K_F9,
    K_F10,
    K_F11,
    K_F12,
    K_PRINTSCREEN,
    K_SCROLLOCK,
    K_PAUSE,
    K_INSERT,
    K_HOME,
    K_PAGEUP,
    K_END,
    K_PAGEDOWN,
    K_RIGHTARROW,
    K_LEFTARROW,
    K_DOWNARROW,
    K_UPARROW,
    K_NUMLOCKCLEAR,
    K_KP_DIVIDE,
    K_KP_MULTIPLY,
    K_KP_PLUS,
    K_KP_ENTER,
    K_KP_1,
    K_KP_2,
    K_KP_3,
    K_KP_4,
    K_KP_5,
    K_KP_6,
    K_KP_7,
    K_KP_8,
    K_KP_9,
    K_KP_0,
    K_KP_PERIOD,
    K_APPLICATION,
    K_POWER,
    K_KP_EQUALS,
    K_F13,
    K_F14,
    K_F15,
    K_F16,
    K_F17,
    K_F18,
    K_F19,
    K_F20,
    K_F21,
    K_F22,
    K_F23,
    K_F24,
    K_EXECUTE,
    K_HELP,
    K_MENU,
    K_SELECT,
    K_STOP,
    K_AGAIN,
    K_UNDO,
    K_CUT,
    K_COPY,
    K_PASTE,
    K_FIND,
    K_MUTE,
    K_VOLUMEUP,
    K_VOLUMEDOWN,
    K_KP_COMMA,
    
    K_LCTRL,
    K_LSHIFT,
    K_LALT,
    K_LGUI,
    K_RCTRL,
    K_RSHIFT,
    K_RALT,
    K_RGUI,
    
    Keycode_Count
};

enum MouseButtons {
    MB_Mouse1 = 0,
    MB_Mouse2 = 1,
    MB_Mouse3 = 2,
    MB_Mouse4 = 3,
    MB_Mouse5 = 4,
    
    MB_Count,
    
    MB_Left = MB_Mouse1,
    MB_Right = MB_Mouse2,
    MB_Middle = MB_Mouse3,
};

struct Event {
    EventType type;
    union {
        struct {
            KeyboardEventType type;
            u8 key;
            bool isRepeat;
		} key;
		struct {
            MouseEventType type;
            
            s32 moveDeltaX;
            s32 moveDeltaY;
            
			s32 posX;
			s32 posY;
            
			s32 scrollDelta;
            
            u8 buttonId;
		} mouse;
		struct {
            GamepadEventType type;
            union {
                struct {
                    u8 id;
                } button;
                struct {
                    u8 id;
                    s16 value;
                } axis;
            };
		} gamepad;
		struct {
			u32 width;
			u32 height;
		} surfaceResize;
	};
};

typedef void(*EventListener)(const Event* event, void* data);

struct EventListenerEntry {
    EventListener callback;
    void* data;
};

struct EventBus {
	DynArray<EventListenerEntry> listeners;
    
	void AddEventListener(EventListener listener, void* data = NULL);
	void PushEvent(const Event* event);
};

EventBus MakeEventBus();
void DestroyEventBus(EventBus* eventBus);