#pragma once

#include "vk_types.h"
#include "VkBootstrap.h"

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

    vkb::InstanceDispatchTable _instance_dispatch_table;
    vkb::DispatchTable _dispatch_table;

    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // debug output handle
    VkPhysicalDevice _chosen_GPU;              // selected GPU
    VkDevice _device;                          // device for commands
    VkSurfaceKHR _surface;                     // window surface

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format;

    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkExtent2D _swapchain_extent;

  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
};