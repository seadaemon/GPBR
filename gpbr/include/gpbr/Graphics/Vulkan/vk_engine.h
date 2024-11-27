#pragma once

#include "vk_types.h"
#include <vector>
#include "vma/vk_mem_alloc.h"
#include <deque>
#include <functional>
#include "vk_descriptors.h"
#include "vk_loader.h"
#include "../camera.h"
#include "../light.h"

// A deque that stores function callbacks.
// Executes in first-in-last-out order.
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

// Contains the structures needed to draw a frame.
struct FrameData
{
    VkSemaphore _swapchain_semaphore, _render_semaphore;
    VkFence _render_fence;

    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer;

    DeletionQueue _deletion_queue;
    DescriptorAllocatorGrowable _frame_descriptors;
};

// Specifies the number of frames to overlap.
// ``FRAME_OVERLAP = 2`` implies double buffering
constexpr unsigned int FRAME_OVERLAP = 2;

// Contains uniform data which can be passed to a compute shader.
struct ComputePushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

// Contains strutures needed to use a compute shader.
struct ComputeEffect
{
    const char* name;

    VkPipeline pipeline;
    VkPipelineLayout layout;

    ComputePushConstants data;
};

// Contains structures required to use glTF PBR materials.
struct GLTFMetallic_Roughness
{
    MaterialPipeline opaque_pipeline;
    MaterialPipeline transparent_pipeline;
    MaterialPipeline mask_pipeline;

    VkDescriptorSetLayout material_layout;

    struct MaterialConstants
    {
        glm::vec4 color_factors;
        glm::vec4 metal_rough_factors;
        glm::vec4 extra[14]; // padding
    };

    struct MaterialResources
    {
        AllocatedImage color_image;
        VkSampler color_sampler;
        AllocatedImage metal_rough_image;
        VkSampler metal_rough_sampler;
        VkBuffer data_buffer;
        uint32_t data_buffer_offset;
    };

    DescriptorWriter writer;

    void build_pipelines(VulkanEngine* engine);
    void clear_resources(VkDevice device);

    MaterialInstance write_material(VkDevice device,
                                    MaterialPass pass,
                                    const MaterialResources& resources,
                                    DescriptorAllocatorGrowable& descriptor_allocator);
};

// Contains necessary data structures for a single vkCmdDrawIndexed command.
struct RenderObject
{
    uint32_t index_count;
    uint32_t first_index;
    VkBuffer index_buffer;

    MaterialInstance* material;
    Bounds bounds;
    glm::mat4 transform;
    VkDeviceAddress vertex_buffer_address;
};

// Contains lists of RenderObjects which can be used for drawing.
struct DrawContext
{
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
    std::vector<RenderObject> mask_surfaces;
};

// Contains statistics related to engine performance
struct EngineStats
{
    float frame_time;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};

// Holds a shared pointer to a MeshAsset
struct MeshNode : public Node
{
    std::shared_ptr<MeshAsset> mesh;

    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx) override;
};

// Uses dynamic rendering to display 3D graphics
class VulkanEngine
{
  public:
    EngineStats stats;
    bool _is_initialized{false};
    bool _stop_rendering{false};
    bool _resize_requested{false};

    struct SDL_Window* _window{nullptr};
    std::string _window_title{"GPBR"};
    VkExtent2D _window_extent{1700, 900};

    // The number of frames elapsed since the start of the program.
    int _frame_number{0};

    static VulkanEngine& get();

    VkInstance _instance;                      // Vulkan library handle.
    VkDebugUtilsMessengerEXT _debug_messenger; // Debugging output handle.
    VkPhysicalDevice _chosen_GPU;              // The selected GPU.
    VkDevice _device;                          // Logical device for commands.
    VkSurfaceKHR _surface;                     // Main window surface.
    VkQueue _graphics_queue;                   // A queue used to submit work
    uint32_t _graphics_queue_family;           // Describes a set of VkQueue with common properties.

    // Maximum number of samples supported by the current physical device
    VkSampleCountFlagBits _msaa_samples{VK_SAMPLE_COUNT_1_BIT};

    FrameData _frames[FRAME_OVERLAP]; // An array of FrameData objects.

    FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };

    VkSwapchainKHR _swapchain; // Manages a list of image buffers that the GPU draws into
    VkFormat _swapchain_image_format;

    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkExtent2D _swapchain_extent;
    VkExtent2D _draw_extent;
    // Allows for dynamic resolution scaling.
    // Do not exeed 1.0f.
    float _render_scale{1.0f};

    // time spent on a single frame (not just draw time)
    float _frame_time{0.0f};
    // rolling average Frames Per Second
    float _avg_FPS{0.0f};
    // 1 second delay between updates
    float _FPS_delay{1.0f};

    // Allocates memory for all descriptors.
    DescriptorAllocatorGrowable global_descriptor_allocator;

    VkPipeline _gradient_pipeline;              // Intended for the gradient compute shaders.
    VkPipelineLayout _gradient_pipeline_layout; // Intended for the gradient compute shaders.

    VkPipeline _mesh_pipeline;              // Intended for 3D meshes
    VkPipelineLayout _mesh_pipeline_layout; // Intended for 3D meshes

    // std::vector<VkFramebuffer> _framebuffers;

    VkDescriptorSet _draw_image_descriptors;
    VkDescriptorSetLayout _draw_image_descriptor_layout;

    VkDescriptorSetLayout _single_image_descriptor_layout;

    DeletionQueue _main_deletion_queue;

    VmaAllocator _allocator;

    // immediate rendering submit structures

    VkFence _imm_fence;
    VkCommandBuffer _imm_command_buffer;
    VkCommandPool _imm_command_pool;

    AllocatedImage _draw_image;  // Main draw image
    AllocatedImage _depth_image; // Main depth image

    AllocatedImage _white_image;              // 1x1 image.
    AllocatedImage _black_image;              // 1x1 image.
    AllocatedImage _grey_image;               // 1x1 image.
    AllocatedImage _error_checkerboard_image; // A magenta and black checkerboard image.

    VkSampler _default_sampler_linear;  // linar filtering (blur)
    VkSampler _default_sampler_nearest; // nearest neighbor filtering

    // draw resources

    DrawContext _main_draw_context;
    GPUSceneData _scene_data;
    MaterialInstance _default_data;

    GLTFMetallic_Roughness _metal_rough_material;

    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loaded_scenes;

    VkDescriptorSetLayout _gpu_scene_data_descriptor_layout;
    VkDescriptorSetLayout _light_data_descriptor_layout;
    // VkDescriptorSetLayout _gltf_mat_descriptor_layout;

    std::vector<ComputeEffect> background_effects;
    int current_background_effect{0};

    // A first-person camera.
    Camera _main_camera;

    // A point light.
    GPULightData _light_data;

    // Initializes structures and objects required to run the engine.
    void init();

    // Destroys all objects and then terminates the program.
    void cleanup();

    // The outermost draw loop.
    // Typically used to call other draw loops.
    void draw();

    // Draws the current scene.
    void draw_main(VkCommandBuffer cmd);
    // Draws a background using compute shaders.
    void draw_background(VkCommandBuffer cmd);
    // Draws 2D/3D geometry.
    void draw_geometry(VkCommandBuffer cmd);
    // Draws ImGui windows using immediate rendering.
    void draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view);
    // Updates the state of the current scene and its objects.
    void update_scene();

    // A single-threaded loop which handles user input and draw calls.
    void run();

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    AllocatedBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void destroy_buffer(const AllocatedBuffer& buffer);

    AllocatedImage create_image(VkExtent3D size,
                                VkFormat format,
                                VkImageUsageFlags usage,
                                bool mipmapped    = false,
                                bool multisampled = false);
    AllocatedImage create_image(void* data,
                                VkExtent3D size,
                                VkFormat format,
                                VkImageUsageFlags usage,
                                bool mipmapped    = false,
                                bool multisampled = false);
    void destroy_image(const AllocatedImage& image);

  private:
    // Initializes:
    // instance, surface, devices, graphics queues, and the Vulkan Memory Allocator.
    void init_vulkan();
    // Creates a swapchain with its images and image views.
    // This overwrites the current swapchain.
    void create_swapchain(uint32_t width, uint32_t height);
    // Initializes the swapchain for the current window.
    void init_swapchain();
    void destroy_swapchain();
    // Recreates the swapchain to suit the current window dimensions.
    void resize_swapchain();

    void init_commands();

    void init_pipelines();
    void init_background_pipelines();

    void init_mesh_pipeline();

    void init_descriptors();

    void init_sync_structures();

    void init_imgui();
    // Initializes objects used for debugging.
    void init_default_data();
};