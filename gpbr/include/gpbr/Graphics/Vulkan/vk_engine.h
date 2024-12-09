/* vk_engine.h
 *
 * Defines the core functionality of renderer.
 * Handles initialization, looping, and destruction.
 *
 */
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

// Implements a queue to store destructor functions.
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) { deletors.push_back(function); }

    // Calls each object in the queue (FILO) and then clears the queue.
    void flush()
    {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)();
        }

        deletors.clear();
    }
};

// A group of data structures required to draw a frame.
struct FrameData
{
    VkSemaphore _swapchain_semaphore, _render_semaphore;
    VkFence _render_fence;

    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer;

    DeletionQueue _deletion_queue;
    DescriptorAllocatorGrowable _frame_descriptors;
};

constexpr unsigned int FRAME_OVERLAP = 2;

// Uniform data to be used in compute shaders.
struct ComputePushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

// Describes a compute shader pipeline.
struct ComputeEffect
{
    const char* name;

    VkPipeline pipeline;
    VkPipelineLayout layout;

    ComputePushConstants data;
};

// Contains structures and pipelines needed to draw metallic-roughness materials. (glTF 2.0)
struct GLTFMetallic_Roughness
{
    MaterialPipeline opaque_pipeline;
    MaterialPipeline transparent_pipeline;
    MaterialPipeline mask_pipeline;

    VkDescriptorSetLayout material_layout;

    struct MaterialConstants
    {
        glm::vec4 base_color_factor;
        float metallic_factor;
        float roughness_factor;
        uint32_t color_tex_ID;
        uint32_t metal_rough_tex_ID;

        glm::vec4 extra[14]; // padding to reach 256 bytes
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

    // Creates opaque, transparent, and mask pipelines.
    void build_pipelines(VulkanEngine* engine);
    // Destroys all pipelines and layouts.
    void clear_resources(VkDevice device);
    // Creates and allocates new material instance.
    MaterialInstance write_material(VkDevice device,
                                    MaterialPass pass,
                                    const MaterialResources& resources,
                                    DescriptorAllocatorGrowable& descriptor_allocator);
};

// Contains necessary data structures for a single vkCmdDrawIndexed call.
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

// Contains lists of RenderObjects to be drawn.
struct DrawContext
{
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
    std::vector<RenderObject> mask_surfaces;
};

// Contains statistics related to engine performance.
struct EngineStats
{
    float frame_time;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};

// A drawable node containing mesh data.
struct MeshNode : public Node
{
    std::shared_ptr<MeshAsset> mesh;
    // Appends render objects to the draw context.
    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx) override;
};

// The index of a given texture.
struct TextureID
{
    uint32_t index;
};

// Manages descriptor images.
struct TextureCache
{
    // List of descriptor image information.
    std::vector<VkDescriptorImageInfo> cache;
    std::unordered_map<std::string, TextureID> name_map;
    // Creates a descriptor texture and returns a corresponding ID.
    TextureID add_texture(const VkImageView& image_view, VkSampler sampler, const std::string& name);
    // Returns the ID of a texture if it exists.
    TextureID find_texture(const std::string& name);
};

// A SDL3/Vulkan 1.3 renderer. Handles initialization, resource management,
// and the main render loop until the window is closed.
class VulkanEngine
{
  public:
    EngineStats stats;
    bool _is_initialized{false};
    bool _stop_rendering{false};
    bool _resize_requested{false};
    int _frame_number{0};

    struct SDL_Window* _window{nullptr};
    std::string _window_title{"GPBR"};
    VkExtent2D _window_extent{1700, 900};

    static VulkanEngine& get();

    VkInstance _instance;                      // Vulkan library handle.
    VkDebugUtilsMessengerEXT _debug_messenger; // Debugging output handle.
    VkPhysicalDevice _chosen_GPU;              // Selected physical device.
    VkDevice _device;                          // Logical device.
    VkSurfaceKHR _surface;                     // Main window surface.
    VkQueue _graphics_queue;                   // Main queue used to submit work.
    uint32_t _graphics_queue_family;           // Describes a set of queues with common properties.

    // Maximum number of samples supported by the current GPU.
    VkSampleCountFlagBits _msaa_samples{VK_SAMPLE_COUNT_1_BIT};

    FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame() { return _frames[_frame_number % FRAME_OVERLAP]; };
    FrameData& get_last_frame() { return _frames[(_frame_number - 1) % FRAME_OVERLAP]; }

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format;
    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;
    VkExtent2D _swapchain_extent;

    VkExtent2D _draw_extent;
    // Allows for dynamic resolution scaling.
    // Do not exeed 1.0f.
    float _render_scale{1.0f};

    // Allocates memory for all descriptors.
    DescriptorAllocatorGrowable global_descriptor_allocator;

    // Pipeline for the default gradient compute shader.
    VkPipeline _gradient_pipeline;
    VkPipelineLayout _gradient_pipeline_layout;

    // Pipeline for drawing 3D meshes.
    VkPipeline _mesh_pipeline;
    VkPipelineLayout _mesh_pipeline_layout;

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
    VkDescriptorSetLayout _gpu_light_data_descriptor_layout;

    // Descriptor layout for bindless rendering pipeline.
    VkDescriptorSetLayout _bindless_descriptor_layout;

    // VkDescriptorSetLayout _gltf_mat_descriptor_layout;

    std::vector<ComputeEffect> background_effects;
    int current_background_effect{0};

    // A first-person camera.
    Camera _main_camera;

    // A point light.
    GPULightData _light_data;

    // New Stuff
    // =======================================

    AllocatedBuffer _default_GLTF_material_data;
    VkDescriptorPool _descriptor_pool;
    TextureCache _texture_cache;

    // =======================================

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