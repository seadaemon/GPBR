#pragma once
#include "vk_types.h"
#include "vk_descriptors.h"
#include <unordered_map>
#include <filesystem>

// forward declaration
class VulkanEngine;

// Bounding box
struct Bounds
{
    glm::vec3 origin;
    float sphere_radius;
    glm::vec3 extents;
};

struct GLTFMaterial
{
    MaterialInstance data;
};

// Geometric surface
struct GeoSurface
{
    uint32_t start_index;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

// Describes the resources of single glTF file.
struct LoadedGLTF : public IRenderable
{
    // glTF file data

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    std::vector<std::shared_ptr<Node>> top_nodes; // aka root nodes

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptor_pool;

    AllocatedBuffer material_data_buffer;

    VulkanEngine* creator; // VulkanEngine instance where this glTF was loaded

    ~LoadedGLTF() { clear_all(); }

    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx);

  private:
    void clear_all();
};

std::optional<std::shared_ptr<LoadedGLTF>> load_gltf(VulkanEngine* engine, std::string_view file_path);