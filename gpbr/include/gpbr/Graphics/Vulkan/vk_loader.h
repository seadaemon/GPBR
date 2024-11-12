#pragma once

#include "vk_types.h"
#include <unordered_map>
#include <filesystem>

class VulkanEngine; // forward declaration

// Geometric surface
struct GeoSurface
{
    uint32_t start_index;
    uint32_t count;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(VulkanEngine* engine,
                                                                        std::filesystem::path file_path);