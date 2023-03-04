#pragma once

inline u8 GetKeycodeFromSdlKeycode(SDL_Keycode key)
{
    if (key < 128) { // ASCII range
        return key;
    }
    else if (key == SDLK_LCTRL) { // ~Hack athon entrant
        return K_LCTRL;
    }
    else if (key == SDLK_LSHIFT) { // ~Hack athon entrant
        return K_LSHIFT;
    }
    else if (key == SDLK_LEFT) { // ~Hack athon entrant
        return K_LEFTARROW;
    }
    else if (key == SDLK_RIGHT) { // ~Hack athon entrant
        return K_RIGHTARROW;
    }
    else {
        // ~Incomplete outside of ascii range
        LogWarn("SDL keycode is out of ascii range, not implemented! %d");
        return 0;
    }
}
