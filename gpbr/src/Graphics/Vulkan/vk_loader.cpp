// #include "stb_image.h"
#include <iostream>
#include "gpbr/Graphics/Vulkan/vk_loader.h"

#include "gpbr/Graphics/Vulkan/vk_engine.h"
#include "gpbr/Graphics/Vulkan/vk_initializers.h"
#include "gpbr/Graphics/Vulkan/vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(VulkanEngine* engine,
                                                                        std::filesystem::path file_path)
{
    fmt::println("Loading GLTF: {}", file_path.generic_string());

    auto data = fastgltf::GltfDataBuffer::FromPath(file_path);
    if (data.error() != fastgltf::Error::None)
    {
        fmt::println("Failed to load GLTF: {}", file_path.generic_string());
        return {};
    }

    constexpr auto gltf_options = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    // Attempt to load the glTF binary with the specified options
    auto load = parser.loadGltfBinary(data.get(), file_path.parent_path(), gltf_options);
    if (load)
    {
        gltf = std::move(load.get());
    }
    else
    {
        fmt::println("Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
        return {};
    }

    // Loop over each mesh, copy vertices & indices into temporary std::vector
    // Then upload them as a mesh to the engine.

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        MeshAsset new_mesh;
        new_mesh.name = mesh.name;

        // clear vectors before processing the mesh
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) // && indicates a rvalue reference
        {
            GeoSurface new_surface;
            new_surface.start_index = (uint32_t)indices.size();
            new_surface.count       = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& index_accessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf, index_accessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
            }

            // load vertex positions
            {
                fastgltf::Accessor& pos_accessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + pos_accessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf,
                                                              pos_accessor,
                                                              [&](glm::vec3 v, size_t index)
                                                              {
                                                                  Vertex new_vtx;
                                                                  new_vtx.position              = v;
                                                                  new_vtx.normal                = {1, 0, 0};
                                                                  new_vtx.color                 = glm::vec4{1.f};
                                                                  new_vtx.uv_x                  = 0;
                                                                  new_vtx.uv_y                  = 0;
                                                                  vertices[initial_vtx + index] = new_vtx;
                                                              });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf,
                                                              gltf.accessors[(*normals).accessorIndex],
                                                              [&](glm::vec3 v, size_t index)
                                                              { vertices[initial_vtx + index].normal = v; });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf,
                                                              gltf.accessors[(*uv).accessorIndex],
                                                              [&](glm::vec2 v, size_t index)
                                                              {
                                                                  vertices[initial_vtx + index].uv_x = v.x;
                                                                  vertices[initial_vtx + index].uv_y = v.y;
                                                              });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf,
                                                              gltf.accessors[(*colors).accessorIndex],
                                                              [&](glm::vec4 v, size_t index)
                                                              { vertices[initial_vtx + index].color = v; });
            }
            new_mesh.surfaces.push_back(new_surface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors)
        {
            for (Vertex& vtx : vertices)
            {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        new_mesh.mesh_buffers = engine->upload_mesh(indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(new_mesh)));
    }
    return meshes;
}