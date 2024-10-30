//==> INCLUDES
#include "gpbr/Graphics/Vulkan/vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <gpbr/Graphics/Vulkan/vk_initializers.h>
#include <gpbr/Graphics/Vulkan/vk_types.h>

#include <chrono>
#include <thread>
//<== INCLUDES

//==> INIT
constexpr bool use_validation_layers = false;

VulkanEngine* loaded_engine = nullptr;

VulkanEngine& VulkanEngine::get()
{
    return *loaded_engine;
}

void VulkanEngine::init()
{
    // the application must only have one engine
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    // initialize SDL & create a window
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        // TODO add error handling here
        fmt::println("Could not initialize SDL: {}", SDL_GetError());
        return;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow("Vulkan Engine", _window_extent.width, _window_extent.height, window_flags);

    _is_initialized = true;
}
//<== INIT

void VulkanEngine::cleanup()
{
    if (_is_initialized)
    {
        SDL_DestroyWindow(_window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::draw() {}

//==> DRAW LOOP
void VulkanEngine::run()
{
    SDL_Event e;
    bool quit = false;

    // main loop
    while (!quit)
    {
        // Poll until all events are handled
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            //==> WINDOW EVENTS
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                _stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED)
            {
                _stop_rendering = false;
            }
            //<== WINDOW EVENTS

            //==> INPUT EVENTS
            if (e.key.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_SPACE)
                {
                    fmt::print("[Space]");
                }
            }
            //<== INPUT EVENTS
        }

        // update game state, draw current frame

        // stop drawing when minimized
        if (_stop_rendering)
        {
            // reduce number of calls to 10/sec
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}
//<== DRAW LOOP