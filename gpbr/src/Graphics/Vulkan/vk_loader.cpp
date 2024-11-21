#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <variant>
#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_loader.h"

#include "gpbr/Graphics/Vulkan/vk_engine.h"
#include "gpbr/Graphics/Vulkan/vk_initializers.h"
#include "gpbr/Graphics/Vulkan/vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image)
{
    AllocatedImage newImage{};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor{
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath)
            {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath());   // We're only capable of loading
                                                      // local files.

                const std::string path(filePath.uri.path().begin(),
                                       filePath.uri.path().end()); // Thanks C++.
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data)
                {
                    VkExtent3D imagesize;
                    imagesize.width  = width;
                    imagesize.height = height;
                    imagesize.depth  = 1;

                    newImage = engine->create_image(
                        data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector)
            {
                unsigned char* data = stbi_load_from_memory((unsigned char*)vector.bytes.data(),
                                                            static_cast<int>(vector.bytes.size()),
                                                            &width,
                                                            &height,
                                                            &nrChannels,
                                                            4);
                if (data)
                {
                    VkExtent3D imagesize;
                    imagesize.width  = width;
                    imagesize.height = height;
                    imagesize.depth  = 1;

                    newImage = engine->create_image(
                        data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view)
            {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer     = asset.buffers[bufferView.bufferIndex];

                // We only care about VectorWithMime here, because we
                // specify LoadExternalBuffers, meaning all buffers
                // are already loaded into a vector.
                std::visit(fastgltf::visitor{[](auto& arg) { std::cout << "!monostate!" << std::endl; },
                                             [&](fastgltf::sources::Array& array)
                                             {
                                                 unsigned char* data = stbi_load_from_memory(
                                                     (unsigned char*)array.bytes.data() + bufferView.byteOffset,
                                                     static_cast<int>(bufferView.byteLength),
                                                     &width,
                                                     &height,
                                                     &nrChannels,
                                                     4);
                                                 if (data)
                                                 {
                                                     VkExtent3D imagesize;
                                                     imagesize.width  = width;
                                                     imagesize.height = height;
                                                     imagesize.depth  = 1;

                                                     newImage = engine->create_image(data,
                                                                                     imagesize,
                                                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                                                     VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                                     false);

                                                     stbi_image_free(data);
                                                 }
                                             }},
                           buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    if (newImage.image == VK_NULL_HANDLE)
    {
        return {};
    }
    else
    {
        return newImage;
    }
}

// Helper: converts filter properties from OpenGL to Vulkan
// Returns VK_FILTER_LINEAR by default
VkFilter extract_filter(fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return VK_FILTER_NEAREST;

    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_FILTER_LINEAR;
    }
}

// Helper: converts mipmap properties from OpenGL to Vulkan
// Returns VK_SAMPLER_MIPMAP_MODE_LINEAR by default
VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::optional<std::shared_ptr<LoadedGLTF>> load_gltf(VulkanEngine* engine, std::string_view file_path)
{
    fmt::println("Loading glTF: {}", file_path);

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator                    = engine;
    LoadedGLTF& file                  = *scene.get();

    fastgltf::Parser parser{};

    constexpr auto gltf_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                  fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    // fastgltf::GltfDataBuffer data;
    // data.FromPath(file_path);

    auto data = fastgltf::GltfDataBuffer::FromPath(file_path);

    if (data.get_if() == nullptr)
    {
        fmt::println("Failed to load glTF: {}", file_path);
        return {};
    }

    fastgltf::Asset gltf;

    std::filesystem::path path = file_path;

    auto type = fastgltf::determineGltfFileType(data.get());

    if (type == fastgltf::GltfType::glTF)
    {
        auto load = parser.loadGltf(data.get(), path.parent_path(), gltf_options);
        if (load)
        {
            gltf = std::move(load.get());
        }
        else
        {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }
    else if (type == fastgltf::GltfType::GLB)
    {
        auto load = parser.loadGltfBinary(data.get(), path.parent_path(), gltf_options);
        if (load)
        {
            gltf = std::move(load.get());
        }
        else
        {
            std::cerr << "Failed to load glTF binary (GLB): " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }
    else
    {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }

    //= Initialize the descriptor pool =========================================

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
        {        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };

    file.descriptor_pool.init(engine->_device, gltf.materials.size(), sizes);

    //= Load samplers ==========================================================
    // Defaults to nearest-neighbor for filtering and mipmapping

    for (fastgltf::Sampler& sampler : gltf.samplers)
    {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod              = VK_LOD_CLAMP_NONE;
        sampl.minLod              = 0;

        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler new_sampler;

        vkCreateSampler(engine->_device, &sampl, nullptr, &new_sampler);

        file.samplers.push_back(new_sampler);
    }

    // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    //= load all textures (use error texture by default) =======================

    // TODO: Use different criteria to differentiate textures

    int ic = 0; // image counter
    for (fastgltf::Image& image : gltf.images)
    {
        std::optional<AllocatedImage> img = load_image(engine, gltf, image);

        if (img.has_value())
        {
            images.push_back(*img);

            file.images[(image.name + char(ic)).c_str()] = *img;
        }
        else
        {
            // use checkerboard texture in case of failure
            images.push_back(engine->_error_checkerboard_image);
            std::cout << "gltf failed to load texture " << image.name << std::endl;
        }
        ic++;
    }

    //= load materials =========================================================

    // create buffer to hold material data
    file.material_data_buffer =
        engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU);
    int data_index = 0;
    GLTFMetallic_Roughness::MaterialConstants* scene_material_constants =
        (GLTFMetallic_Roughness::MaterialConstants*)file.material_data_buffer.info.pMappedData;

    // load all materials
    for (fastgltf::Material& mat : gltf.materials)
    {
        // add new_mat to the unordered_map (materials)
        std::shared_ptr<GLTFMaterial> new_mat = std::make_shared<GLTFMaterial>();
        materials.push_back(new_mat);
        file.materials[mat.name.c_str()] = new_mat;

        // gather material constants
        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.color_factors.x       = mat.pbrData.baseColorFactor[0];
        constants.color_factors.y       = mat.pbrData.baseColorFactor[1];
        constants.color_factors.z       = mat.pbrData.baseColorFactor[2];
        constants.color_factors.w       = mat.pbrData.baseColorFactor[3];
        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;

        // get the alpha cut-off
        constants.extra[0].x = mat.alphaCutoff;

        // write material parameters to buffer
        scene_material_constants[data_index] = constants;

        // determine if material supports transparency
        MaterialPass pass_type = MaterialPass::MainColor;

        if (mat.alphaMode == fastgltf::AlphaMode::Blend)
        {
            pass_type = MaterialPass::Transparent;
        }
        else if (mat.alphaMode == fastgltf::AlphaMode::Mask)
        {
            pass_type = MaterialPass::Mask;
        }

        GLTFMetallic_Roughness::MaterialResources material_resources;
        // default the material textures (white)
        material_resources.color_image         = engine->_white_image;
        material_resources.color_sampler       = engine->_default_sampler_linear;
        material_resources.metal_rough_image   = engine->_white_image;
        material_resources.metal_rough_sampler = engine->_default_sampler_linear;

        // set the uniform buffer for the material data
        material_resources.data_buffer        = file.material_data_buffer.buffer;
        material_resources.data_buffer_offset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);

        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value())
        {
            size_t img     = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            material_resources.color_image   = images[img];
            material_resources.color_sampler = file.samplers[sampler];
        }
        // build material
        new_mat->data = engine->_metal_rough_material.write_material(
            engine->_device, pass_type, material_resources, file.descriptor_pool);

        data_index++;
    }

    //= load meshes ============================================================
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        // add the new_mesh to the mesh unordered_map

        std::shared_ptr<MeshAsset> new_mesh = std::make_shared<MeshAsset>();
        meshes.push_back(new_mesh);
        file.meshes[mesh.name.c_str()] = new_mesh;
        new_mesh->name                 = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives)
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
                                                                  Vertex newvtx;
                                                                  newvtx.position               = v;
                                                                  newvtx.normal                 = {1, 0, 0};
                                                                  newvtx.color                  = glm::vec4{1.f};
                                                                  newvtx.uv_x                   = 0;
                                                                  newvtx.uv_y                   = 0;
                                                                  vertices[initial_vtx + index] = newvtx;
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

            if (p.materialIndex.has_value())
            {
                new_surface.material = materials[p.materialIndex.value()];
            }
            else
            {
                new_surface.material = materials[0];
            }

            glm::vec3 minpos = vertices[initial_vtx].position;
            glm::vec3 maxpos = vertices[initial_vtx].position;
            for (int i = initial_vtx; i < vertices.size(); i++)
            {
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }

            new_surface.bounds.origin        = (maxpos + minpos) / 2.f;
            new_surface.bounds.extents       = (maxpos - minpos) / 2.f;
            new_surface.bounds.sphere_radius = glm::length(new_surface.bounds.extents);
            new_mesh->surfaces.push_back(new_surface);
        }

        new_mesh->mesh_buffers = engine->upload_mesh(indices, vertices);
    }

    //= load nodes and their meshes ============================================
    for (fastgltf::Node& node : gltf.nodes)
    {
        std::shared_ptr<Node> new_node;

        // allocate MeshNode object if node has a mesh
        if (node.meshIndex.has_value())
        {
            new_node                                     = std::make_shared<MeshNode>();
            static_cast<MeshNode*>(new_node.get())->mesh = meshes[*node.meshIndex];
        }
        else
        {
            new_node = std::make_shared<Node>();
        }

        nodes.push_back(new_node);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix)
                                     { memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix)); },
                                     [&](fastgltf::TRS transform)
                                     {
                                         glm::vec3 tl(transform.translation[0],
                                                      transform.translation[1],
                                                      transform.translation[2]);
                                         glm::quat rot(transform.rotation[3],
                                                       transform.rotation[0],
                                                       transform.rotation[1],
                                                       transform.rotation[2]);
                                         glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         new_node->local_transform = tm * rm * sm;
                                     }},
                   node.transform);
    }

    // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node              = gltf.nodes[i];
        std::shared_ptr<Node>& scene_node = nodes[i];

        for (auto& c : node.children)
        {
            scene_node->children.push_back(nodes[c]);
            nodes[c]->parent = scene_node;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes)
    {
        if (node->parent.lock() == nullptr)
        {
            file.top_nodes.push_back(node);
            node->refresh_transform(glm::mat4{1.f});
        }
    }
    return scene;
}

void LoadedGLTF::draw(const glm::mat4& top_matrix, DrawContext& ctx)
{
    // create renderables from scene nodes
    for (auto& n : top_nodes)
    {
        n->draw(top_matrix, ctx);
    }
}

void LoadedGLTF::clear_all()
{
    VkDevice dv = creator->_device;

    for (auto& [k, v] : meshes)
    {
        creator->destroy_buffer(v->mesh_buffers.index_buffer);
        creator->destroy_buffer(v->mesh_buffers.vertex_buffer);
    }

    for (auto& [k, v] : images)
    {

        if (v.image == creator->_error_checkerboard_image.image)
        {
            // dont destroy the default images
            continue;
        }
        creator->destroy_image(v);
    }

    for (auto& sampler : samplers)
    {
        vkDestroySampler(dv, sampler, nullptr);
    }

    descriptor_pool.destroy_pools(dv);

    creator->destroy_buffer(material_data_buffer);
}