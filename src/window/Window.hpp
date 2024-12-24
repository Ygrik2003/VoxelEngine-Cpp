#pragma once

#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <memory>
#include <stack>
#include <vector>

#include "typedefs.hpp"

class ImageData;
struct DisplaySettings;
struct GLFWwindow;
struct GLFWmonitor;

class Window {
    static SDL_Window* window;
    static DisplaySettings* settings;
    static std::stack<glm::vec4> scissorStack;
    static glm::vec4 scissorArea;
    static bool fullscreen;
    static bool to_quit;
    static int framerate;
    static double prevSwap;

    static bool tryToMaximize(SDL_Window* window, SDL_DisplayID monitor);
public:
    static int posX;
    static int posY;
    static uint width;
    static uint height;
    static int initialize(DisplaySettings* settings);
    static void terminate();

    static void viewport(int x, int y, int width, int height);
    static void setRelativeMouseMode(bool mode);
    static bool isShouldClose();
    static void setShouldClose(bool flag);
    static void swapBuffers();
    static void setFramerate(int interval);
    static void toggleFullscreen();
    static bool isFullscreen();
    static bool isMaximized();
    static bool isFocused();
    static bool isIconified();

    static void pushScissor(glm::vec4 area);
    static void popScissor();
    static void resetScissor();

    static void clear();
    static void clearDepth();
    static void setBgColor(glm::vec3 color);
    static void setBgColor(glm::vec4 color);
    static double time();
    static const char* getClipboardText();
    static void setClipboardText(const char* text);
    static DisplaySettings* getSettings();
    static void setIcon(const ImageData* image);

    static glm::vec2 size() {
        return glm::vec2(width, height);
    }

    static std::unique_ptr<ImageData> takeScreenshot();
};
