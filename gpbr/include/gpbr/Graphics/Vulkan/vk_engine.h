#pragma once

#include "vk_types.h"
#include <vector>
#include "vma/vk_mem_alloc.h"
#include <deque>
#include <functional>
#include "vk_descriptors.h"
#include "vk_loader.h"

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
    DescriptorAllocatorGrowable _frame_descriptors;
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

// Contains glTF PBR parameters
struct GLTFMetallic_Roughness
{
    MaterialPipeline opaque_pipeline;
    MaterialPipeline transparent_pipeline;

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

// Contains data for a single vkCmdDrawIndexed
struct RenderObject
{
    uint32_t index_count;
    uint32_t first_index;
    VkBuffer index_buffer;

    MaterialInstance* material;

    glm::mat4 transform;
    VkDeviceAddress vertex_buffer_address;
};

struct DrawContext
{
    std::vector<RenderObject> opaque_surfaces;
};

struct MeshNode : public Node
{
    std::shared_ptr<MeshAsset> mesh;

    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx) override;
};

class VulkanEngine
{
  public:
    bool _is_initialized{false};
    int _frame_number{0}; // The number of frames elapsed since launch
    bool _stop_rendering{false};
    VkExtent2D _window_extent{1700, 900};
    // VkExtent2D _window_extent{1920, 1080};
    bool _resize_requested{false};

    struct SDL_Window* _window{nullptr};
    std::string _window_title;

    static VulkanEngine& get();

    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // debugging output handle
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
    float _render_scale{1.0f};

    // Query pool for time stamps to calculate draw time
    VkQueryPool _query_pool_time_stamps = VK_NULL_HANDLE;
    std::vector<uint64_t> _time_stamps;
    float _timestamp_period = 0; // obtained from the GPU

    // time spent on a single frame (not just draw time)
    float _frame_time{0.0f};
    // rolling average Frames Per Second
    float _avg_FPS{0.0f};
    // 1 second delay between updates
    float _FPS_delay{1.0f};

    DescriptorAllocatorGrowable global_descriptor_allocator;

    VkPipeline _gradient_pipeline;
    VkPipelineLayout _gradient_pipeline_layout;
    /*
    std::vector<VkFramebuffer> _framebuffers;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    */
    VkDescriptorSet _draw_image_descriptors;
    VkDescriptorSetLayout _draw_image_descriptor_layout;

    VkDescriptorSetLayout _single_image_descriptor_layout;

    DeletionQueue _main_deletion_queue;

    VmaAllocator _allocator;

    VkPipeline _mesh_pipeline;
    VkPipelineLayout _mesh_pipeline_layout;

    std::vector<std::shared_ptr<MeshAsset>> test_meshes;

    // immediate submit structures
    VkFence _imm_fence;
    VkCommandBuffer _imm_command_buffer;
    VkCommandPool _imm_command_pool;

    AllocatedImage _draw_image;
    AllocatedImage _depth_image;

    AllocatedImage _white_image;
    AllocatedImage _black_image;
    AllocatedImage _grey_image;
    AllocatedImage _error_checkerboard_image;

    VkSampler _default_sampler_linear;  // linar filtering (blur)
    VkSampler _default_sampler_nearest; // nearest neighbor filtering

    // draw resources

    DrawContext _main_draw_context;
    GPUSceneData _scene_data;
    MaterialInstance _default_data;

    GLTFMetallic_Roughness _metal_rough_material;

    std::unordered_map<std::string, std::shared_ptr<Node>> _loaded_nodes;

    VkDescriptorSetLayout _gpu_scene_data_descriptor_layout;
    // VkDescriptorSetLayout _gltf_mat_descriptor_layout;

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

    void update_scene();

    // run main loop
    void run();

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    AllocatedBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void destroy_buffer(const AllocatedBuffer& buffer);

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data,
                                VkExtent3D size,
                                VkFormat format,
                                VkImageUsageFlags usage,
                                bool mipmapped = false);
    void destroy_image(const AllocatedImage& image);

  private:
    void init_vulkan();

    void init_swapchain();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
    void resize_swapchain();

    void init_commands();

    void init_queries();

    void init_pipelines();
    void init_background_pipelines();

    void init_mesh_pipeline();

    void init_descriptors();

    void init_sync_structures();

    void init_imgui();

    void init_default_data();
};