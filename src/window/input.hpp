#pragma once

#include <string>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

#include "util/RunnablesList.hpp"

/// @brief Represents SDL keycode values.
enum class keycode : int {
    SPACE = SDLK_SPACE,
    APOSTROPHE = SDLK_APOSTROPHE,
    COMMA = SDLK_COMMA,
    MINUS = SDLK_MINUS,
    PERIOD = SDLK_PERIOD,
    SLASH = SDLK_SLASH,
    NUM_0 = SDLK_KP_0,
    NUM_1 = SDLK_KP_1,
    NUM_2 = SDLK_KP_2,
    NUM_3 = SDLK_KP_3,
    NUM_4 = SDLK_KP_4,
    NUM_5 = SDLK_KP_5,
    NUM_6 = SDLK_KP_6,
    NUM_7 = SDLK_KP_7,
    NUM_8 = SDLK_KP_8,
    NUM_9 = SDLK_KP_9,
    SEMICOLON = SDLK_SEMICOLON,
    EQUAL = SDLK_EQUALS,
    A = SDLK_A,
    B = SDLK_B,
    C = SDLK_C,
    D = SDLK_D,
    E = SDLK_E,
    F = SDLK_F,
    G = SDLK_G,
    H = SDLK_H,
    I = SDLK_I,
    J = SDLK_J,
    K = SDLK_K,
    L = SDLK_L,
    M = SDLK_M,
    N = SDLK_N,
    O = SDLK_O,
    P = SDLK_P,
    Q = SDLK_Q,
    R = SDLK_R,
    S = SDLK_S,
    T = SDLK_T,
    U = SDLK_U,
    V = SDLK_V,
    W = SDLK_W,
    X = SDLK_X,
    Y = SDLK_Y,
    Z = SDLK_Z,
    LEFT_BRACKET = SDLK_LEFTBRACKET,
    BACKSLASH = SDLK_BACKSLASH,
    RIGHT_BRACKET = SDLK_RIGHTBRACKET,
    GRAVE_ACCENT = SDLK_GRAVE,
    ESCAPE = SDLK_ESCAPE,
    ENTER = SDLK_RETURN,
    TAB = SDLK_TAB,
    BACKSPACE = SDLK_BACKSPACE,
    INSERT = SDLK_INSERT,
    DELETE = SDLK_DELETE,
    LEFT = SDLK_LEFT,
    RIGHT = SDLK_RIGHT,
    DOWN = SDLK_DOWN,
    UP = SDLK_UP,
    PAGE_UP = SDLK_PAGEUP,
    PAGE_DOWN = SDLK_PAGEDOWN,
    HOME = SDLK_HOME,
    END = SDLK_END,
    CAPS_LOCK = SDLK_CAPSLOCK,
    NUM_LOCK = SDLK_NUMLOCKCLEAR,
    PRINT_SCREEN = SDLK_PRINTSCREEN,
    PAUSE = SDLK_PAUSE,
    F1 = SDLK_F1,
    F2 = SDLK_F2,
    F3 = SDLK_F3,
    F4 = SDLK_F4,
    F5 = SDLK_F5,
    F6 = SDLK_F6,
    F7 = SDLK_F7,
    F8 = SDLK_F8,
    F9 = SDLK_F9,
    F10 = SDLK_F10,
    F11 = SDLK_F11,
    F12 = SDLK_F12,
    LEFT_SHIFT = SDLK_LSHIFT,
    LEFT_CONTROL = SDLK_LCTRL,
    LEFT_ALT = SDLK_LALT,
    LEFT_SUPER = SDLK_LGUI,
    RIGHT_SHIFT = SDLK_RSHIFT,
    RIGHT_CONTROL = SDLK_RCTRL,
    RIGHT_ALT = SDLK_RALT,
    RIGHT_SUPER = SDLK_RGUI,
    MENU = SDLK_MENU,
    UNKNOWN = SDLK_UNKNOWN
};

/// @brief Represents glfw3 mouse button IDs.
/// @details There is a subset of glfw3 mouse button IDs.
enum class mousecode : int {
    BUTTON_1 = SDL_BUTTON_LEFT,  // Left mouse button
    BUTTON_2 = SDL_BUTTON_RIGHT,  // Right mouse button
    BUTTON_3 = SDL_BUTTON_MIDDLE,  // Middle mouse button
    UNKNOWN = -1,
};

inline mousecode MOUSECODES_ALL[] {
    mousecode::BUTTON_1, mousecode::BUTTON_2, mousecode::BUTTON_3};

namespace input_util {
    void initialize();

    keycode keycode_from(const std::string& name);
    mousecode mousecode_from(const std::string& name);

    /// @return Key label by keycode
    std::string to_string(keycode code);
    /// @return Mouse button label by keycode
    std::string to_string(mousecode code);

    /// @return Key name by keycode
    std::string get_name(keycode code);
    /// @return Mouse button name by keycode
    std::string get_name(mousecode code);
}

enum class inputtype {
    keyboard,
    mouse,
};

struct Binding {
    util::RunnablesList onactived;

    inputtype type;
    int code;
    bool state = false;
    bool justChange = false;
    bool enable = true;

    Binding() = default;
    Binding(inputtype type, int code) : type(type), code(code) {
    }

    bool active() const {
        return state;
    }

    bool jactive() const {
        return state && justChange;
    }

    void reset(inputtype, int);
    void reset(keycode);
    void reset(mousecode);

    inline std::string text() const {
        switch (type) {
            case inputtype::keyboard: {
                return input_util::to_string(static_cast<keycode>(code));
            }
            case inputtype::mouse: {
                return input_util::to_string(static_cast<mousecode>(code));
            }
        }
        return "<unknown input type>";
    }
};
