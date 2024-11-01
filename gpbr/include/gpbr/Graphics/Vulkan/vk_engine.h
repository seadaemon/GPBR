#pragma once

#include "vk_types.h"
#include "VkBootstrap.h"

struct FrameData
{
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine
{
  public:
    bool _is_initialized{false};
    int _frame_number{0};
    VkExtent2D _window_extent{1700, 900};

    struct SDL_Window* _window{nullptr};

    static VulkanEngine& get();

    vkb::InstanceDispatchTable _instance_dispatch_table;
    vkb::DispatchTable _dispatch_table;

    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // debug output handle
    VkPhysicalDevice _chosen_GPU;              // selected GPU
    VkDevice _device;                          // device for commands
    VkSurfaceKHR _surface;                     // window surface

    FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };

    VkQueue _graphics_queue;
    uint32_t _graphics_queue_family;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format;

    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkExtent2D _swapchain_extent;

    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup();

    // draw loop
    void draw();

    // run main loop
    void run();

    bool _stop_rendering{false};

  private:
    void init_vulkan();
    void init_swapchain();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();

    void init_commands();

    void init_sync_structures();
};