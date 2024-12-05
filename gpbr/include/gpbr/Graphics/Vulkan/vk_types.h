/* vk_types.h
 *
 * Provides common include directives and structures. Intended to be used
 * with source files in the Vulkan directory.
 *
 */
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vma/vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/trigonometric.hpp>

// Represents a Vulkan image and its related resources.
struct AllocatedImage
{
    VkImage image;
    VkImageView image_view;
    VmaAllocation allocation;
    VkExtent3D image_extent;
    VkFormat image_format;
};

// Represents a Vulkan buffer and its related resources.
struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

// Scene data to be sent to the GPU as a uniform buffer.
struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;
    glm::vec3 camera_pos;
    glm::vec4 ambient_color;
    glm::vec4 sunlight_direction; // xyz for direction; w for intensity
    glm::vec4 sunlight_color;
};
static_assert(sizeof(GPUSceneData) <= 256);

// Material pass type to determine which pipeline to use.
enum class MaterialPass : uint8_t
{
    MainColor,
    Transparent,
    Mask,
    Other
};

// Contains a pipeline and pipeline layout for a given material.
struct MaterialPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

// Contains a material pipeline, descriptor set, and material pass for a given material.
struct MaterialInstance
{
    MaterialPipeline* pipeline;
    VkDescriptorSet material_set;
    MaterialPass pass_type;
};

// Contains vertex data to be sent to the GPU.
struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

// Contains mesh-specific index and vertex buffers to be sent to the GPU.
struct GPUMeshBuffers
{
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

// Contains mesh-specific data to be used in a draw-call. Intended to be sent
// to the GPU as a push constant.
struct GPUDrawPushConstants
{
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer_address;
};
static_assert(sizeof(GPUDrawPushConstants) <= 128);

struct DrawContext; // forward decl.

// Base class for a renderable object
class IRenderable
{
    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx) = 0;
};

// Implementation of a drawable scene node.
// Supports hierarchical transformations.
struct Node : public IRenderable
{
    std::weak_ptr<Node> parent; // Weak pointer to avoid circular dependencies.
    std::vector<std::shared_ptr<Node>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    // Recursively updates the world transform for each node.
    void refresh_transform(const glm::mat4& parent_matrix)
    {
        world_transform = parent_matrix * local_transform;
        for (auto c : children)
        {
            c->refresh_transform(world_transform);
        }
    }

    // Recursively draws each node.
    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx)
    {
        for (auto& c : children)
        {
            c->draw(top_matrix, ctx);
        }
    }
};

// CONSIDER: add support for error values less than 0 (e.g. if(err <= 0))

#define VK_CHECK(x)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult err = x;                                                                                              \
        if (err)                                                                                                       \
        {                                                                                                              \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err));                                             \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)