#include "Window.hpp"

#include <GL/glew.h>
#include <SDL3/SDL.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "Events.hpp"
#include "debug/Logger.hpp"
#include "graphics/core/ImageData.hpp"
#include "graphics/core/Texture.hpp"
#include "settings.hpp"
#include "util/ObjectsKeeper.hpp"
#include "util/platform.hpp"

static debug::Logger logger("window");

SDL_Window* Window::window = nullptr;
DisplaySettings* Window::settings = nullptr;
std::stack<glm::vec4> Window::scissorStack;
glm::vec4 Window::scissorArea;
uint Window::width = 0;
uint Window::height = 0;
int Window::posX = 0;
int Window::posY = 0;
int Window::framerate = -1;
double Window::prevSwap = 0.0;
bool Window::fullscreen = false;
bool Window::to_quit = true;

static util::ObjectsKeeper observers_keeper;

void cursor_position_callback(SDL_Window*, double xpos, double ypos) {
    Events::setPosition(xpos, ypos);
}

void mouse_button_callback(SDL_Window*, int button, int action, int) {
    Events::setButton(button, action == SDL_EVENT_MOUSE_BUTTON_DOWN);
}

// SDL_AppResult process_event(SDL_Event* event) {
//     switch (event->type) {
//         case SDL_EVENT_QUIT:
//             // Window::to_quit = true;
//             break;
//         case SDL_EVENT_KEY_DOWN:
//             Events::setKey(event->key.key, false);
//             break;
//         case SDL_EVENT_KEY_UP:
//             Events::setKey(event->key.key, true);
//             Events::pressedKeys.push_back(static_cast<keycode>(event->key.key));
//             break;
//         case SDL_EVENT_WINDOW_RESIZED:
//             break;
//     }
// }

void scroll_callback(SDL_Window*, double xoffset, double yoffset) {
    Events::scroll += yoffset;
}

bool Window::isMaximized() {
    return SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
}

bool Window::isIconified() {
    return SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
}

bool Window::isFocused() {
    return SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS;
}

void window_size_callback(SDL_Window*, int width, int height) {
    if (width && height) {
        glViewport(0, 0, width, height);
        Window::width = width;
        Window::height = height;

        if (!Window::isFullscreen() && !Window::isMaximized()) {
            Window::getSettings()->width.set(width);
            Window::getSettings()->height.set(height);
        }
    }
    Window::resetScissor();
}

void character_callback(SDL_Window*, unsigned int codepoint) {
    Events::codepoints.push_back(codepoint);
}

const char* glfwErrorName(int error) {
    // switch (error) {
    //     case GLFW_NO_ERROR:
    //         return "no error";
    //     case GLFW_NOT_INITIALIZED:
    //         return "not initialized";
    //     case GLFW_NO_CURRENT_CONTEXT:
    //         return "no current context";
    //     case GLFW_INVALID_ENUM:
    //         return "invalid enum";
    //     case GLFW_INVALID_VALUE:
    //         return "invalid value";
    //     case GLFW_OUT_OF_MEMORY:
    //         return "out of memory";
    //     case GLFW_API_UNAVAILABLE:
    //         return "api unavailable";
    //     case GLFW_VERSION_UNAVAILABLE:
    //         return "version unavailable";
    //     case GLFW_PLATFORM_ERROR:
    //         return "platform error";
    //     case GLFW_FORMAT_UNAVAILABLE:
    //         return "format unavailable";
    //     case GLFW_NO_WINDOW_CONTEXT:
    //         return "no window context";
    //     default:
    return "unknown error";
    // }
}

void error_callback(int error, const char* description) {
    // std::cerr << "GLFW error [0x" << std::hex << error << "]: ";
    // std::cerr << glfwErrorName(error) << std::endl;
    // if (description) {
    //     std::cerr << description << std::endl;
    // }
}

int Window::initialize(DisplaySettings* settings) {
    Window::settings = settings;
    Window::width = settings->width.get();
    Window::height = settings->height.get();

    std::string title = "VoxelCore v" + std::to_string(ENGINE_VERSION_MAJOR) +
                        "." + std::to_string(ENGINE_VERSION_MINOR);
    if (ENGINE_DEBUG_BUILD) {
        title += " [debug]";
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef __APPLE__
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE
    );
#else
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
    );
#endif
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, settings->samples.get());

    window = SDL_CreateWindow(
        title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        logger.error() << "failed to create SDL window";
        SDL_Quit();
        return -1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint maxTextureSize[1] {static_cast<GLint>(Texture::MAX_RESOLUTION)};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);
    if (maxTextureSize[0] > 0) {
        Texture::MAX_RESOLUTION = maxTextureSize[0];
        logger.info() << "max texture size is " << Texture::MAX_RESOLUTION;
    }

    observers_keeper = util::ObjectsKeeper();
    observers_keeper.keepAlive(settings->fullscreen.observe(
        [](bool value) {
            if (value != isFullscreen()) {
                toggleFullscreen();
            }
        },
        true
    ));

    SDL_GL_SetSwapInterval(1);  // VSync
    setFramerate(settings->framerate.get());
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    logger.info() << "GL Vendor: " << reinterpret_cast<const char*>(vendor);
    logger.info() << "GL Renderer: " << reinterpret_cast<const char*>(renderer);
    logger.info() << "SDL: " << SDL_GetVersion();
    glm::vec2 scale;
    scale.x = scale.y = SDL_GetWindowDisplayScale(window);
    logger.info() << "monitor content scale: " << scale.x << "x" << scale.y;

    input_util::initialize();
    return 0;
}

void Window::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::clearDepth() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Window::setBgColor(glm::vec3 color) {
    glClearColor(color.r, color.g, color.b, 1.0f);
}

void Window::setBgColor(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void Window::viewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void Window::setRelativeMouseMode(bool mode) {
    SDL_CaptureMouse(mode);
}

void Window::resetScissor() {
    scissorArea = glm::vec4(0.0f, 0.0f, width, height);
    scissorStack = std::stack<glm::vec4>();
    glDisable(GL_SCISSOR_TEST);
}

void Window::pushScissor(glm::vec4 area) {
    if (scissorStack.empty()) {
        glEnable(GL_SCISSOR_TEST);
    }
    scissorStack.push(scissorArea);

    area.z += glm::ceil(area.x);
    area.w += glm::ceil(area.y);

    area.x = glm::max(area.x, scissorArea.x);
    area.y = glm::max(area.y, scissorArea.y);

    area.z = glm::min(area.z, scissorArea.z);
    area.w = glm::min(area.w, scissorArea.w);

    if (area.z < 0.0f || area.w < 0.0f) {
        glScissor(0, 0, 0, 0);
    } else {
        glScissor(
            area.x,
            Window::height - area.w,
            std::max(0, static_cast<int>(glm::ceil(area.z - area.x))),
            std::max(0, static_cast<int>(glm::ceil(area.w - area.y)))
        );
    }
    scissorArea = area;
}

void Window::popScissor() {
    if (scissorStack.empty()) {
        logger.warning() << "extra Window::popScissor call";
        return;
    }
    glm::vec4 area = scissorStack.top();
    scissorStack.pop();
    if (area.z < 0.0f || area.w < 0.0f) {
        glScissor(0, 0, 0, 0);
    } else {
        glScissor(
            area.x,
            Window::height - area.w,
            std::max(0, int(area.z - area.x)),
            std::max(0, int(area.w - area.y))
        );
    }
    if (scissorStack.empty()) {
        glDisable(GL_SCISSOR_TEST);
    }
    scissorArea = area;
}

void Window::terminate() {
    observers_keeper = util::ObjectsKeeper();
    SDL_Quit();
}

bool Window::isShouldClose() {
    return to_quit;
}

void Window::setShouldClose(bool flag) {
    // glfwSetWindowShouldClose(window, flag);
}

void Window::setFramerate(int framerate) {
    if ((framerate != -1) != (Window::framerate != -1)) {
        SDL_GL_SetSwapInterval(framerate == -1);
    }
    Window::framerate = framerate;
}

void Window::toggleFullscreen() {
    fullscreen = !fullscreen;

    SDL_DisplayID monitor = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(monitor);

    if (Events::_cursor_locked) Events::toggleCursor();

    if (fullscreen) {
        // glfwGetWindowPos(window, &posX, &posY);
        // glfwSetWindowMonitor(
        //     window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE
        // );
    } else {
        // glfwSetWindowMonitor(
        //     window,
        //     nullptr,
        //     posX,
        //     posY,
        //     settings->width.get(),
        //     settings->height.get(),
        //     GLFW_DONT_CARE
        // );
    }

    float xPos, yPos;
    SDL_WarpMouseInWindow(window, xPos, yPos);
    Events::setPosition(xPos, yPos);
}

bool Window::isFullscreen() {
    return fullscreen;
}

void Window::swapBuffers() {
    SDL_GL_SwapWindow(window);
    Window::resetScissor();
    if (framerate > 0) {
        auto elapsedTime = time() - prevSwap;
        auto frameTime = 1.0 / framerate;
        if (elapsedTime < frameTime) {
            platform::sleep(
                static_cast<size_t>((frameTime - elapsedTime) * 1000)
            );
        }
    }
    prevSwap = time();
}

/// @return Current time in seconds
double Window::time() {
    SDL_Time ticks;
    if (!SDL_GetCurrentTime(&ticks)) {
        logger.error() << "SDL error when getting current time: "
                       << SDL_GetError();
    }
    return reinterpret_cast<int64_t>(ticks) * 1e-9;
}

DisplaySettings* Window::getSettings() {
    return settings;
}

std::unique_ptr<ImageData> Window::takeScreenshot() {
    auto data = std::make_unique<ubyte[]>(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.get());
    return std::make_unique<ImageData>(
        ImageFormat::rgb888, width, height, data.release()
    );
}

const char* Window::getClipboardText() {
    return SDL_GetClipboardText();
}

void Window::setClipboardText(const char* text) {
    SDL_SetClipboardText(text);
}

bool Window::tryToMaximize(SDL_Window* window, SDL_DisplayID monitor) {
    glm::ivec4 windowFrame(0);
    glm::ivec4 workArea(0);

    SDL_GetWindowSize(window, &windowFrame.z, &windowFrame.w);
    SDL_GetWindowPosition(window, &windowFrame.x, &windowFrame.y);

    const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(monitor);
    workArea.z = displayMode->w;
    workArea.w = displayMode->h;

    if (Window::width > (uint)workArea.z) Window::width = (uint)workArea.z;
    if (Window::height > (uint)workArea.w) Window::height = (uint)workArea.w;
    if (Window::width >= (uint)(workArea.z - (windowFrame.x + windowFrame.z)) &&
        Window::height >=
            (uint)(workArea.w - (windowFrame.y + windowFrame.w))) {
        SDL_MaximizeWindow(window);
        return true;
    }
    SDL_SetWindowSize(window, Window::width, Window::height);
    SDL_SetWindowPosition(
        window,
        (workArea.z - Window::width) / 2,
        (workArea.w - Window::height) / 2 + windowFrame.y / 2
    );
    return false;
}

void Window::setIcon(const ImageData* image) {
    SDL_PixelFormat currPixelFormat;
    switch (image->getFormat()) {
        case ImageFormat::rgb888:
            currPixelFormat = SDL_PixelFormat::SDL_PIXELFORMAT_RGB24;
            break;
        case ImageFormat::rgba8888:
            currPixelFormat = SDL_PixelFormat::SDL_PIXELFORMAT_RGBA8888;
            break;
    }
    SDL_Surface* icon = SDL_CreateSurface(
        static_cast<int>(image->getWidth()),
        static_cast<int>(image->getHeight()),
        currPixelFormat
    );
    icon->pixels = static_cast<void*>(image->getData());

    if (!SDL_SetWindowIcon(window, icon)) {
        logger.error() << "SDL cant set icon for app with next error: "
                       << SDL_GetError();
    }
}
