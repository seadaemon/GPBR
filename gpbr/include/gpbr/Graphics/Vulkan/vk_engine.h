#pragma once

#include "vk_types.h"
#include <vector>
#include "vma/vk_mem_alloc.h"
#include <deque>
#include <functional>
#include "vk_descriptors.h"

// A FILO queue that stores function callbacks
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) { deletors.push_back(function); }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)(); // call functors
        }

        deletors.clear();
    }
};

struct FrameData
{
    VkSemaphore _swapchain_semaphore, _render_semaphore;
    VkFence _render_fence;

    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer;

    DeletionQueue _deletion_queue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct ComputePushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

struct ComputeEffect
{
    const char* name;

    VkPipeline pipeline;
    VkPipelineLayout layout;

    ComputePushConstants data;
};

class VulkanEngine
{
  public:
    bool _is_initialized{false};
    int _frame_number{0};
    bool _stop_rendering{false};
    VkExtent2D _window_extent{1700, 900};

    struct SDL_Window* _window{nullptr};

    static VulkanEngine& get();

    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // debug output handle
    VkPhysicalDevice _chosen_GPU;              // selected GPU
    VkDevice _device;                          // device for commands
    VkSurfaceKHR _surface;                     // window surface
    VkQueue _graphics_queue;
    uint32_t _graphics_queue_family;

    FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format;

    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkExtent2D _swapchain_extent;
    VkExtent2D _draw_extent;

    DescriptorAllocator global_descriptor_allocator;

    VkPipeline _gradient_pipeline;
    VkPipelineLayout _gradient_pipeline_layout;
    /*
    std::vector<VkFramebuffer> _framebuffers;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    */
    VkDescriptorSet _draw_image_descriptors;
    VkDescriptorSetLayout _draw_image_descriptor_layout;

    DeletionQueue _main_deletion_queue;

    VmaAllocator _allocator;

    VkPipeline _triangle_pipeline;
    VkPipelineLayout _triangle_pipeline_layout;

    // immediate submit structures
    VkFence _imm_fence;
    VkCommandBuffer _imm_command_buffer;
    VkCommandPool _imm_command_pool;

    AllocatedImage _draw_image;

    std::vector<ComputeEffect> background_effects;

    int current_background_effect{0};

    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup();

    // draw loop
    void draw();

    void draw_background(VkCommandBuffer cmd);
    void draw_geometry(VkCommandBuffer cmd);
    void draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view);

    // run main loop
    void run();

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

  private:
    void init_vulkan();

    void init_swapchain();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();

    void init_commands();

    void init_pipelines();
    void init_background_pipelines();

    void init_triangle_pipeline();

    void init_descriptors();

    void init_sync_structures();

    void init_imgui();
};