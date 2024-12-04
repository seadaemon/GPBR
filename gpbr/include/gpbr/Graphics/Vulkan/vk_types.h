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

// s
struct AllocatedImage
{
    VkImage image;
    VkImageView image_view;
    VmaAllocation allocation;
    VkExtent3D image_extent;
    VkFormat image_format;
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

// Unused?
struct GPUGLTFMaterial
{
    glm::vec4 color_factors;
    glm::vec4 metal_rough_factors;
    glm::vec4 extra[14];
};

static_assert(sizeof(GPUGLTFMaterial) == 256);

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

enum class MaterialPass : uint8_t
{
    MainColor,
    Transparent,
    Mask,
    Other
};

// Specifies a pipeline and pipeline layout for a given material
struct MaterialPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

// Specifies a pipeline and descriptor set for a given material
struct MaterialInstance
{
    MaterialPipeline* pipeline;
    VkDescriptorSet material_set;
    MaterialPass pass_type;
};

struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

// Holds the resources needed for a mesh
struct GPUMeshBuffers
{
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

// push constants for mesh object draws
struct GPUDrawPushConstants
{
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
};

static_assert(sizeof(GPUDrawPushConstants) <= 128);

// TODO: implement this?
struct DrawContext;

// Base class for a renderable dynamic object
class IRenderable
{
    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx) = 0;
};

// Implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct Node : public IRenderable
{

    // parent pointer must be a weak pointer to avoid circular dependencies
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    void refresh_transform(const glm::mat4& parent_matrix)
    {
        world_transform = parent_matrix * local_transform;
        for (auto c : children)
        {
            c->refresh_transform(world_transform);
        }
    }

    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx)
    {
        // draw children
        for (auto& c : children)
        {
            c->draw(top_matrix, ctx);
        }
    }
};

// TODO: add support for error values less than 0 (e.g. if(err <= 0))
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