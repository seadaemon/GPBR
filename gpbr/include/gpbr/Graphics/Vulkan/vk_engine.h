#pragma once

#include "vk_types.h"

class VulkanEngine
{
  public:
    bool _is_initialized{false};
    int _frame_number{0};
    bool _stop_rendering{false};
    VkExtent2D _window_extent{1700, 900};

    struct SDL_Window* _window{nullptr};

    static VulkanEngine& get();

    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup();

    // draw loop
    void draw();

    // run main loop
    void run();
};