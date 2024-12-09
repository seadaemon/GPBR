/* vk_loader.h
 *
 * Provides functionality to load 3D models (meshes, scenes of meshes, etc.).
 * Primarily focuses on the glTF 2.0 standard.
 *
 */
#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"
#include <unordered_map>
#include <filesystem>

class VulkanEngine; // forward declaration

// Oriented bounding box (OBB).
struct Bounds
{
    glm::vec3 origin;
    float sphere_radius;
    glm::vec3 extents;
};

// Contains material instances relating to a GLTF material.
struct GLTFMaterial
{
    MaterialInstance data;
    // MaterialInstance shadow_data;
};

// Geometric surface of a mesh.
struct GeoSurface
{
    uint32_t start_index;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

// A mesh composed of one or more surfaces.
struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

// A renderable object derived from a glTF file.
struct LoadedGLTF : public IRenderable
{
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    std::vector<std::shared_ptr<Node>> top_nodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptor_pool;

    AllocatedBuffer material_data_buffer; // Contains material constants.

    VulkanEngine* creator;

    ~LoadedGLTF() { clear_all(); }

    // Recursively draws each node.
    virtual void draw(const glm::mat4& top_matrix, DrawContext& ctx);

  private:
    void clear_all();
};

// Loads mesh data from a glTF/glb file and creates a LoadedGLTF object if successful.
std::optional<std::shared_ptr<LoadedGLTF>> load_gltf(VulkanEngine* engine, std::string_view file_path);