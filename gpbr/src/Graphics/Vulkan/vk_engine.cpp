#define VOLK_IMPLEMENTATION
#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <gpbr/Graphics/Vulkan/vk_types.h>
#include <gpbr/Graphics/Vulkan/vk_initializers.h>

#include "VkBootstrap.h"
#include <array>
#include <chrono>
#include <thread>
#include <fstream>
#include <numeric>

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "gpbr/Util/imgui_util.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
/*
#ifndef VMA_DEBUG_LOG_FORMAT
#define VMA_DEBUG_LOG_FORMAT(format, ...)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        printf((format), __VA_ARGS__);                                                                                 \
        printf("\n");                                                                                                  \
    } while (false)
#endif

#ifndef VMA_DEBUG_LOG
#ifndef VMA_ENABLE_DEBUG_LOG
/// VMA debug log disabled
#define VMA_DEBUG_LOG(str)
#else
/// VMA debug log enabled
#define VMA_DEBUG_LOG(str) VMA_DEBUG_LOG_FORMAT("%s", (str))
#endif
#endif
*/
#include "vma/vk_mem_alloc.h"
#include <gpbr/Graphics/Vulkan/vk_images.h>
#include <gpbr/Graphics/Vulkan/vk_pipelines.h>
#include <gpbr/Graphics/Vulkan/vk_descriptors.h>
#include <glm/gtx/transform.hpp>
constexpr bool use_validation_layers = true;

VulkanEngine* loaded_engine = nullptr;

VulkanEngine& VulkanEngine::get()
{
    return *loaded_engine;
}

void VulkanEngine::init()
{
    /* 1 Ensure this is the only engine running */
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    /* 2 Initialize SDL */
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fmt::println("Could not initialize SDL: {}", SDL_GetError());
        return; // TODO: Improve error handling here...
    }

    /* 2.1 Create the SDL window */

    _window_title = "GPBR";

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE);

    _window = SDL_CreateWindow(_window_title.c_str(), _window_extent.width, _window_extent.height, window_flags);

    /* 3 Process each initialization method */

    init_vulkan();

    init_swapchain();

    init_commands();

    init_queries();

    init_sync_structures();

    init_descriptors();

    init_pipelines();

    init_imgui();

    init_default_data();

    _is_initialized = true;

    /* 4 Initialize the camera */

    _main_camera.velocity = glm::vec3(0.f);
    _main_camera.position = glm::vec3(0.f, 1.76f, 0.f);
    _main_camera.pitch    = 0.f;
    _main_camera.yaw      = 0.f;

    /* 5 Load a scene for debugging */

    std::string prefix{".\\Assets\\"};

    std::unordered_map<std::string, std::string> glTF_map{
        {"AlphaBlendModeTest", prefix + "AlphaBlendModeTest.glb"},
        {               "Cow",                prefix + "Cow.glb"},
        {              "Duck",               prefix + "Duck.glb"},
        {            "Dragon",  prefix + "DragonAttenuation.glb"},
        {            "Sponza",             prefix + "sponza.glb"},
        {         "Structure",          prefix + "structure.glb"},
        {     "Structure Mat",      prefix + "structure_mat.glb"}
    };

    std::string gltf_path{glTF_map["Sponza"]};
    auto gltf_file = load_gltf(this, gltf_path);
    assert(gltf_file.has_value());

    _loaded_scenes["debug"] = *gltf_file;
}

void VulkanEngine::init_default_data()
{
    //= 3 default textures (white, grey, black) 1 pixel for each ===============
    uint32_t white = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
    _white_image =
        create_image((void*)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.666f, 0.666f, 0.666f, 1.0f));
    _grey_image = create_image((void*)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 1.f));
    _black_image =
        create_image((void*)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    //= 16x16 checkerboard image ===============================================
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));
    std::array<uint32_t, 16 * 16> pixels;
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    _error_checkerboard_image =
        create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    VkSamplerCreateInfo sampler_info = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_device, &sampler_info, nullptr, &_default_sampler_nearest);

    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_device, &sampler_info, nullptr, &_default_sampler_linear);

    _main_deletion_queue.push_function(
        [&]()
        {
            vkDestroySampler(_device, _default_sampler_nearest, nullptr);
            vkDestroySampler(_device, _default_sampler_linear, nullptr);

            destroy_image(_white_image);
            destroy_image(_grey_image);
            destroy_image(_black_image);
            destroy_image(_error_checkerboard_image);
        });

    //= glTF PBR default =======================================================

    GLTFMetallic_Roughness::MaterialResources materialResources;
    // default the material textures
    materialResources.color_image         = _white_image;
    materialResources.color_sampler       = _default_sampler_linear;
    materialResources.metal_rough_image   = _white_image;
    materialResources.metal_rough_sampler = _default_sampler_linear;

    // set the uniform buffer for the material data
    AllocatedBuffer materialConstants = create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants),
                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);

    // write the buffer
    GLTFMetallic_Roughness::MaterialConstants* sceneUniformData =
        (GLTFMetallic_Roughness::MaterialConstants*)materialConstants.allocation->GetMappedData();
    sceneUniformData->color_factors       = glm::vec4{1, 1, 1, 1};
    sceneUniformData->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};

    _main_deletion_queue.push_function([=, this]() { destroy_buffer(materialConstants); });

    materialResources.data_buffer        = materialConstants.buffer;
    materialResources.data_buffer_offset = 0;

    _default_data = _metal_rough_material.write_material(
        _device, MaterialPass::MainColor, materialResources, global_descriptor_allocator);

    //= default meshes =========================================================
    test_meshes = load_gltf_meshes(this, ".\\Assets\\basicmesh.glb").value();

    for (auto& m : test_meshes)
    {
        std::shared_ptr<MeshNode> new_node = std::make_shared<MeshNode>();
        new_node->mesh                     = m;

        new_node->local_transform = glm::mat4{1.f};
        new_node->world_transform = glm::mat4{1.f};

        for (auto& s : new_node->mesh->surfaces)
        {
            s.material = std::make_shared<GLTFMaterial>(_default_data);
        }

        _loaded_nodes[m->name] = std::move(new_node);
    }
}

void VulkanEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < _swapchain_image_views.size(); i++)
    {
        vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    }
}

void VulkanEngine::resize_swapchain()
{
    vkDeviceWaitIdle(_device);

    destroy_swapchain();

    int width, height;
    SDL_GetWindowSize(_window, &width, &height);

    _window_extent.width  = width;
    _window_extent.height = height;

    create_swapchain(_window_extent.width, _window_extent.height);

    _resize_requested = false;
}

void VulkanEngine::cleanup()
{
    if (_is_initialized)
    {
        vkDeviceWaitIdle(_device);

        _loaded_scenes.clear();

        _metal_rough_material.clear_resources(_device);

        for (auto& frame : _frames)
        {
            frame._deletion_queue.flush();
        }

        for (auto& mesh : test_meshes)
        {
            destroy_buffer(mesh->mesh_buffers.index_buffer);
            destroy_buffer(mesh->mesh_buffers.vertex_buffer);
        }

        _main_deletion_queue.flush();

        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vmaDestroyAllocator(_allocator);

        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::draw_main(VkCommandBuffer cmd)
{
    ComputeEffect& effect = background_effects[current_background_effect];

    // bind the background compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

    // bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradient_pipeline_layout, 0, 1, &_draw_image_descriptors, 0, nullptr);

    vkCmdPushConstants(
        cmd, _gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, std::ceil(_window_extent.width / 16.0), std::ceil(_window_extent.height / 16.0), 1);

    // draw the triangle

    vkutil::transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colorAttachment =
        vkinit::attachment_info(_draw_image.image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment =
        vkinit::depth_attachment_info(_depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = vkinit::rendering_info(_window_extent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    auto start = std::chrono::system_clock::now();

    draw_geometry(cmd);

    auto end     = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    stats.mesh_draw_time = elapsed.count() / 1000.f;

    vkCmdEndRendering(cmd);
}

void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
    ComputeEffect& effect = background_effects[current_background_effect];

    // bind the background compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

    // bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradient_pipeline_layout, 0, 1, &_draw_image_descriptors, 0, nullptr);

    vkCmdPushConstants(
        cmd, _gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, std::ceil(_draw_extent.width / 16.0), std::ceil(_draw_extent.height / 16.0), 1);
}

bool is_visible(const RenderObject& obj, const glm::mat4& viewproj)
{
    std::array<glm::vec3, 8> corners{
        glm::vec3{ 1,  1,  1},
        glm::vec3{ 1,  1, -1},
        glm::vec3{ 1, -1,  1},
        glm::vec3{ 1, -1, -1},
        glm::vec3{-1,  1,  1},
        glm::vec3{-1,  1, -1},
        glm::vec3{-1, -1,  1},
        glm::vec3{-1, -1, -1},
    };

    glm::mat4 matrix = viewproj * obj.transform;

    glm::vec3 min = {1.5, 1.5, 1.5};
    glm::vec3 max = {-1.5, -1.5, -1.5};

    for (int c = 0; c < 8; c++)
    {
        // project each corner into clip space
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

        // perspective correction
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;

        min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
        max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
    }

    // check the clip space box is within the view
    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(_main_draw_context.opaque_surfaces.size());

    for (int i = 0; i < _main_draw_context.opaque_surfaces.size(); i++)
    {
        /*
        if (is_visible(_main_draw_context.opaque_surfaces[i], _scene_data.view_proj))
        {
            opaque_draws.push_back(i);
        }
        */
        opaque_draws.push_back(i);
    }

    // sort the opaque surfaces by material and mesh
    std::sort(opaque_draws.begin(),
              opaque_draws.end(),
              [&](const auto& iA, const auto& iB)
              {
                  const RenderObject& A = _main_draw_context.opaque_surfaces[iA];
                  const RenderObject& B = _main_draw_context.opaque_surfaces[iB];
                  if (A.material == B.material)
                  {
                      return A.index_buffer < B.index_buffer;
                  }
                  else
                  {
                      return A.material < B.material;
                  }
              });

    // allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer =
        create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // add it to the deletion queue of this frame so it gets deleted once its been used
    get_current_frame()._deletion_queue.push_function([=, this]() { destroy_buffer(gpuSceneDataBuffer); });

    // write the buffer
    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData              = _scene_data;

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        get_current_frame()._frame_descriptors.allocate(_device, _gpu_scene_data_descriptor_layout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_device, globalDescriptor);

    MaterialPipeline* lastPipeline = nullptr;
    MaterialInstance* lastMaterial = nullptr;
    VkBuffer lastIndexBuffer       = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject& r)
    {
        if (r.material != lastMaterial)
        {
            lastMaterial = r.material;
            if (r.material->pipeline != lastPipeline)
            {

                lastPipeline = r.material->pipeline;
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        r.material->pipeline->layout,
                                        0,
                                        1,
                                        &globalDescriptor,
                                        0,
                                        nullptr);

                VkViewport viewport = {};
                viewport.x          = 0;
                viewport.y          = 0;
                viewport.width      = (float)_window_extent.width;
                viewport.height     = (float)_window_extent.height;
                viewport.minDepth   = 0.f;
                viewport.maxDepth   = 1.f;

                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor      = {};
                scissor.offset.x      = 0;
                scissor.offset.y      = 0;
                scissor.extent.width  = _window_extent.width;
                scissor.extent.height = _window_extent.height;

                vkCmdSetScissor(cmd, 0, 1, &scissor);
            }

            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    r.material->pipeline->layout,
                                    1,
                                    1,
                                    &r.material->material_set,
                                    0,
                                    nullptr);
        }
        if (r.index_buffer != lastIndexBuffer)
        {
            lastIndexBuffer = r.index_buffer;
            vkCmdBindIndexBuffer(cmd, r.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        // calculate final mesh matrix
        GPUDrawPushConstants push_constants;
        push_constants.world_matrix  = r.transform;
        push_constants.vertex_buffer = r.vertex_buffer_address;

        vkCmdPushConstants(cmd,
                           r.material->pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(GPUDrawPushConstants),
                           &push_constants);

        stats.drawcall_count++;
        stats.triangle_count += r.index_count / 3;
        vkCmdDrawIndexed(cmd, r.index_count, 1, r.first_index, 0, 0);
    };

    stats.drawcall_count = 0;
    stats.triangle_count = 0;

    for (auto& r : opaque_draws)
    {
        draw(_main_draw_context.opaque_surfaces[r]);
    }

    for (auto& r : _main_draw_context.transparent_surfaces)
    {
        draw(r);
    }

    // we delete the draw commands now that we processed them
    _main_draw_context.opaque_surfaces.clear();
    _main_draw_context.transparent_surfaces.clear();
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view)
{
    VkRenderingAttachmentInfo color_attachment =
        vkinit::attachment_info(target_image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo render_info = vkinit::rendering_info(_swapchain_extent, &color_attachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VulkanEngine::update_scene()
{
    _main_camera.update();

    glm::mat4 view = _main_camera.get_view_matrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(70.0f), (float)_draw_extent.width / (float)_draw_extent.height, 1000.f, 0.1f);

    // invert the Y direction on projection matrix so that we are more similar
    // to opengl and gltf axis
    projection[1][1] *= -1;

    _scene_data.view      = view;
    _scene_data.proj      = projection;
    _scene_data.view_proj = projection * view;

    //=====================================================================================

    _main_draw_context.opaque_surfaces.clear();

    //_loaded_nodes["Suzanne"]->draw(glm::mat4{1.f}, _main_draw_context);

    _loaded_scenes["debug"]->draw(glm::mat4{1.f}, _main_draw_context);

    //_scene_data.view = glm::translate(glm::vec3{0, 0, -5});

    // camera projection
    //_scene_data.proj =
    // glm::perspective(glm::radians(70.f), (float)_window_extent.width / (float)_window_extent.height, 10000.f, 0.1f);

    // invert the Y direction on projection matrix so that we are more similar
    // to opengl and gltf axis
    //_scene_data.proj[1][1] *= -1;
    //_scene_data.view_proj = _scene_data.proj * _scene_data.view;

    // parameters for a directional light
    _scene_data.ambient_color      = glm::vec4(.5f);
    _scene_data.sunlight_color     = glm::vec4(1.f);
    _scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);
}

void VulkanEngine::draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, true, 1000000000));

    get_current_frame()._deletion_queue.flush();
    get_current_frame()._frame_descriptors.clear_pools(_device);

    // request an image from the swapchain
    uint32_t swapchain_image_index;

    VkResult e = vkAcquireNextImageKHR(
        _device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index);

    // VK_ERROR_OUT_OF_DATE_KHR is not guarenteed, so consider multiple cases
    if (e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR)
    {
        _resize_requested = true;
        return;
    }
    if (e != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to acquire next image from the swapchain!\n");
    }

    _draw_extent.width  = std::min(_swapchain_extent.width, _draw_image.image_extent.width) * _render_scale;
    _draw_extent.height = std::min(_swapchain_extent.height, _draw_image.image_extent.height) * _render_scale;

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._render_fence));

    // reset command buffer to begin recording again
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._main_command_buffer, 0));

    // naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._main_command_buffer;

    // begin recording. This command buffer is used exactly once
    VkCommandBufferBeginInfo cmd_begin_info =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //== BEGIN COMMAND BUFFER ========================================

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // reset and start a new timestamp
    vkCmdResetQueryPool(cmd, _query_pool_timestamps, 0, static_cast<uint32_t>(_timestamps.size()));
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _query_pool_timestamps, 0);

    // transition the main draw image into general layout so it can be written to
    vkutil::transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vkutil::transition_image(
        cmd, _depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    // TODO: CONTINUE HERE
    // https://github.com/vblanco20-1/vulkan-guide/blob/dcf72a8b3cf93e27b917639a012be1b4b24b5e7d/chapter-5/vk_engine.cpp#L345
    draw_main(cmd);

    vkutil::transition_image(
        cmd, _draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(
        cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkExtent2D extent;
    extent.height = _window_extent.height;
    extent.width  = _window_extent.width;

    // execute a copy from the draw image into the swapchain
    vkutil::copy_image_to_image(
        cmd, _draw_image.image, _swapchain_images[swapchain_image_index], _draw_extent, _swapchain_extent);

    // set swapchain image layout to Attachment Optimal so we can draw it
    vkutil::transition_image(cmd,
                             _swapchain_images[swapchain_image_index],
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // draw imgui into the swapchain image
    draw_imgui(cmd, _swapchain_image_views[swapchain_image_index]);

    // set swapchain image layout to Present so we can draw it
    vkutil::transition_image(cmd,
                             _swapchain_images[swapchain_image_index],
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _query_pool_timestamps, 1);

    // finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    //== END COMMAND BUFFER ==========================================

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished
    VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                    get_current_frame()._swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info =
        vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._render_semaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution

    VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, get_current_frame()._render_fence));

    // get time stamp results
    uint32_t count = _timestamps.size();

    vkGetQueryPoolResults(_device, //
                          _query_pool_timestamps,
                          0,
                          count,
                          _timestamps.size() * sizeof(uint64_t),
                          _timestamps.data(),
                          sizeof(uint64_t),
                          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    // prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR present_info = {};
    present_info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext            = nullptr;
    present_info.pSwapchains      = &_swapchain;
    present_info.swapchainCount   = 1;

    present_info.pWaitSemaphores    = &get_current_frame()._render_semaphore;
    present_info.waitSemaphoreCount = 1;

    present_info.pImageIndices = &swapchain_image_index;

    VkResult present_result = vkQueuePresentKHR(_graphics_queue, &present_info);

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
    {
        fmt::println("present_result out of date!");
        _resize_requested = true;
        return;
    }

    _frame_number++;
}

void VulkanEngine::run()
{
    // TODO: implement fixed timestep
    // const float FPS = 60.0f;
    // const float dt  = 1.0f / FPS; // ~= 16.6ms

    auto previous_time = std::chrono::high_resolution_clock::now();

    SDL_Event e;
    bool quit = false;
    // main loop
    while (!quit)
    {

        auto start = std::chrono::system_clock::now();

        // Poll until all events are handled
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            //==> WINDOW EVENTS
            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                _resize_requested = true;
            }
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                _stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED)
            {
                _stop_rendering = false;
            }
            //<== WINDOW EVENTS

            _main_camera.process_SDL_event(e);
            ImGui_ImplSDL3_ProcessEvent(&e);

            //==> INPUT EVENTS
            if (e.key.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_SPACE)
                {
                    fmt::print("[Space]");
                }
            }
            //<== INPUT EVENTS
        }

        // update game state, draw current frame

        // stop drawing when minimized
        if (_stop_rendering)
        {
            // reduce number of calls to 10/sec
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (_resize_requested)
        {
            resize_swapchain();
        }
        // Create a new ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Stats");
        ImGui::Text("frametime %f ms", stats.frame_time);
        ImGui::Text("drawtime %f ms", stats.mesh_draw_time);
        ImGui::Text("triangles %i", stats.triangle_count);
        ImGui::Text("drawws %i", stats.drawcall_count);
        ImGui::End();

        if (ImGui::Begin("background"))
        {
            ImGui::SliderFloat("Render Scale", &_render_scale, 0.3f, 1.0f);

            ComputeEffect& selected = background_effects[current_background_effect];

            ImGui::Text("Selected effect: ", selected.name);

            ImGui::SliderInt("Effect Index", &current_background_effect, 0, background_effects.size() - 1);

            ImGui::InputFloat4("data1", (float*)&selected.data.data1);
            ImGui::InputFloat4("data2", (float*)&selected.data.data2);
            ImGui::InputFloat4("data3", (float*)&selected.data.data3);
            ImGui::InputFloat4("data4", (float*)&selected.data.data4);
        }
        ImGui::End();
        // make imgui calculate internal draw structures
        ImGui::Render();

        update_scene();

        draw();

        auto end     = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        stats.frame_time = elapsed.count() / 1000.f;
    }
}

void VulkanEngine::init_vulkan()
{
    /* 1 Create handles to the Vulkan library and the debug messenger */

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("GPBR")
                        .request_validation_layers(use_validation_layers) //
                        .use_default_debug_messenger()                    //
                        .require_api_version(1, 3, 0)                     //
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    _instance              = vkb_inst.instance;
    _debug_messenger       = vkb_inst.debug_messenger;

    /* 2 Create a Vulkan rendering surface */

    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    /* 3 Specify features and extensions required of the physical device  */

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    features13.maintenance4     = true;

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing  = true;
    features12.hostQueryReset      = true;

    // Vulkan 1.0 features
    VkPhysicalDeviceFeatures features{};
    features.fillModeNonSolid = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};

    selector.add_required_extension("VK_KHR_dynamic_rendering");

    selector.add_required_extension("VK_EXT_extended_dynamic_state");

    /* 3.1 Create handles for a physical device and a logical device */

    vkb::PhysicalDevice physical_device = selector                                  //
                                              .set_minimum_version(1, 3)            //
                                              .set_required_features_13(features13) //
                                              .set_required_features_12(features12) //
                                              .set_required_features(features)      //
                                              .set_surface(_surface)                //
                                              .select()                             //
                                              .value();

    vkb::DeviceBuilder device_builder{physical_device};

    vkb::Device vkb_device = device_builder.build().value();

    _device     = vkb_device.device;
    _chosen_GPU = physical_device.physical_device;

    fmt::println("{}\n", physical_device.name);

    /* 4 Use Volk to dynamically load function entrypoints */

    volkInitialize();
    volkLoadInstance(_instance);
    volkLoadDevice(_device);

    /* 5 Obtain a graphics queue and its corresponding family */

    _graphics_queue        = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    /* 6 Bind Vulkan functions for VMA */

    // TODO: Find a more elegant solution
    VmaVulkanFunctions vma_funcs                      = {};
    vma_funcs.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
    vma_funcs.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
    vma_funcs.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vma_funcs.vkAllocateMemory                        = vkAllocateMemory;
    vma_funcs.vkFreeMemory                            = vkFreeMemory;
    vma_funcs.vkMapMemory                             = vkMapMemory;
    vma_funcs.vkUnmapMemory                           = vkUnmapMemory;
    vma_funcs.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vma_funcs.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vma_funcs.vkBindBufferMemory                      = vkBindBufferMemory;
    vma_funcs.vkBindImageMemory                       = vkBindImageMemory;
    vma_funcs.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vma_funcs.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vma_funcs.vkCreateBuffer                          = vkCreateBuffer;
    vma_funcs.vkDestroyBuffer                         = vkDestroyBuffer;
    vma_funcs.vkCreateImage                           = vkCreateImage;
    vma_funcs.vkDestroyImage                          = vkDestroyImage;
    vma_funcs.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vma_funcs.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
    vma_funcs.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
    vma_funcs.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
    vma_funcs.vkBindImageMemory2KHR                   = vkBindImageMemory2;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vma_funcs.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vma_funcs.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

    /* 6.1 Initialize VMA */

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.pVulkanFunctions       = &vma_funcs;
    allocator_info.physicalDevice         = _chosen_GPU;
    allocator_info.device                 = _device;
    allocator_info.instance               = _instance;
    allocator_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocator_info, &_allocator);

    // Destructory for _allocator is explicitly called in cleanup()
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{_chosen_GPU, _device, _surface};

    _swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain =
        swapchain_builder
            .set_desired_format(VkSurfaceFormatKHR{.format     = _swapchain_image_format,            //
                                                   .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}) //
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)                                      // vsync
            .set_desired_extent(width, height)                                                       //
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)                                  //
            .build()                                                                                 //
            .value();

    _swapchain_extent = vkb_swapchain.extent;
    // store swapchain & related images
    _swapchain             = vkb_swapchain.swapchain;
    _swapchain_images      = vkb_swapchain.get_images().value();
    _swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain()
{
    /* 1 Create a new swapchain */

    create_swapchain(_window_extent.width, _window_extent.height);

    /* 2 Set extent and format of the _draw_image */

    VkExtent3D draw_image_extent = {
        2560, //
        1600, //
        1     //
    };        // TODO: make this detect the current monitor resolution

    _draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcoded to 32 bit signed float
    _draw_image.image_extent = draw_image_extent;

    /* 2.1 Define usage flags for the _draw_image */

    VkImageUsageFlags draw_image_usages{};
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /* 2.2 Define creation structs for _draw_image */

    VkImageCreateInfo draw_image_info =
        vkinit::image_create_info(_draw_image.image_format, draw_image_usages, draw_image_extent);

    // Allocate it from gpu local memory
    VmaAllocationCreateInfo draw_image_alloc_info = {};
    draw_image_alloc_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    draw_image_alloc_info.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    /* 2.3 Initialize _draw_image */

    vmaCreateImage(
        _allocator, &draw_image_info, &draw_image_alloc_info, &_draw_image.image, &_draw_image.allocation, nullptr);

    /* 2.4 Build an image-view from the _draw_image */

    VkImageViewCreateInfo rview_info =
        vkinit::imageview_create_info(_draw_image.image_format, _draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_draw_image.image_view));

    /* 3 Initialize the _depth_image */

    _depth_image.image_format = VK_FORMAT_D32_SFLOAT;
    _depth_image.image_extent = draw_image_extent;
    VkImageUsageFlags depth_image_usages{};
    depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info =
        vkinit::image_create_info(_depth_image.image_format, depth_image_usages, draw_image_extent);

    // allocate and create the image
    vmaCreateImage(
        _allocator, &dimg_info, &draw_image_alloc_info, &_depth_image.image, &_depth_image.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info =
        vkinit::imageview_create_info(_depth_image.image_format, _depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depth_image.image_view));

    /* 4 Queue deletion functions for the draw image and depth image */

    _main_deletion_queue.push_function(
        [=]()
        {
            vkDestroyImageView(_device, _draw_image.image_view, nullptr);
            vmaDestroyImage(_allocator, _draw_image.image, _draw_image.allocation);

            vkDestroyImageView(_device, _depth_image.image_view, nullptr);
            vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation);
        });
}

void VulkanEngine::init_commands()
{
    /* 1 Specify the info struct to create command pools */

    // allow for resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_info =
        vkinit::command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    /* 2 Create command pools and command buffers for each frame */

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[i]._command_pool));

        VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(_frames[i]._command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_frames[i]._main_command_buffer));

        _main_deletion_queue.push_function([=]() { vkDestroyCommandPool(_device, _frames[i]._command_pool, nullptr); });
    }

    // immediate commands
    VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_imm_command_pool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(_imm_command_pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_imm_command_buffer));

    _main_deletion_queue.push_function([=]() { vkDestroyCommandPool(_device, _imm_command_pool, nullptr); });
}

void VulkanEngine::init_queries()
{
    // get the timestamp period for the selected GPU
    VkPhysicalDeviceProperties physical_device_properties{};
    vkGetPhysicalDeviceProperties(_chosen_GPU, &physical_device_properties);
    VkPhysicalDeviceLimits device_limits = physical_device_properties.limits;
    _timestamp_period                    = device_limits.timestampPeriod;

    // 2 time points for 1 render pass
    _timestamps.resize(2);

    // create timestamp query pool
    VkQueryPoolCreateInfo query_pool_info =
        vkinit::query_pool_create_info(VK_QUERY_TYPE_TIMESTAMP, static_cast<uint32_t>(_timestamps.size()));
    VK_CHECK(vkCreateQueryPool(_device, &query_pool_info, nullptr, &_query_pool_timestamps));

    _main_deletion_queue.push_function([=]() { vkDestroyQueryPool(_device, _query_pool_timestamps, nullptr); });
}

void VulkanEngine::init_sync_structures()
{
    // create syncronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain
    // we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_imm_fence));

    _main_deletion_queue.push_function([=]() { vkDestroyFence(_device, _imm_fence, nullptr); });

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

        VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));

        _main_deletion_queue.push_function(
            [=]()
            {
                vkDestroyFence(_device, _frames[i]._render_fence, nullptr);
                vkDestroySemaphore(_device, _frames[i]._swapchain_semaphore, nullptr);
                vkDestroySemaphore(_device, _frames[i]._render_semaphore, nullptr);
            });
    }
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(_device, 1, &_imm_fence));
    VK_CHECK(vkResetCommandBuffer(_imm_command_buffer, 0));

    VkCommandBuffer cmd = _imm_command_buffer;

    VkCommandBufferBeginInfo cmd_begin_info =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit               = vkinit::submit_info(&cmd_info, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, _imm_fence));

    VK_CHECK(vkWaitForFences(_device, 1, &_imm_fence, true, 9999999999));
}

void VulkanEngine::init_imgui()
{
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversized, (copied from the imgui demo)
    VkDescriptorPoolSize pool_sizes[] = {
        {               VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 1000;
    pool_info.poolSizeCount              = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes                 = pool_sizes;

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imgui_pool));

    // 2: initialize imgui library
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL3_InitForVulkan(_window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = _instance;
    init_info.PhysicalDevice            = _chosen_GPU;
    init_info.Device                    = _device;
    init_info.Queue                     = _graphics_queue;
    init_info.DescriptorPool            = imgui_pool;
    init_info.MinImageCount             = 3; // FRAME_OVERLAP?
    init_info.ImageCount                = 3;
    init_info.UseDynamicRendering       = true;
    // dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchain_image_format;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    util::set_imgui_theme(ImGui::GetStyle(), "gpbr");

    // add the destroy the imgui created structures
    _main_deletion_queue.push_function(
        [=]()
        {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(_device, imgui_pool, nullptr);
        });
}

void VulkanEngine::init_pipelines()
{
    // COMPUTE PIPELINES
    init_background_pipelines();

    // GRAPHICS PIPELINES
    init_mesh_pipeline();

    // glTF PBR PIPELINES
    _metal_rough_material.build_pipelines(this);
}

void VulkanEngine::init_background_pipelines()
{
    VkPipelineLayoutCreateInfo compute_layout{};
    compute_layout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compute_layout.pNext          = nullptr;
    compute_layout.pSetLayouts    = &_draw_image_descriptor_layout;
    compute_layout.setLayoutCount = 1;

    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = sizeof(ComputePushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    compute_layout.pPushConstantRanges    = &push_constant;
    compute_layout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &compute_layout, nullptr, &_gradient_pipeline_layout));

    VkShaderModule gradient_shader;
    if (!vkutil::load_shader_module("./Shaders/gradient_color.comp.spv", _device, &gradient_shader))
    {
        fmt::print("Error when building the compute shader\n");
    }

    VkShaderModule sky_shader;
    if (!vkutil::load_shader_module("./Shaders/sky.comp.spv", _device, &sky_shader))
    {
        fmt::print("Error when building the compute shader\n");
    }

    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext  = nullptr;
    stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = gradient_shader;
    stage_info.pName  = "main";

    VkComputePipelineCreateInfo compute_pipeline_create_info{};
    compute_pipeline_create_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext  = nullptr;
    compute_pipeline_create_info.layout = _gradient_pipeline_layout;
    compute_pipeline_create_info.stage  = stage_info;

    ComputeEffect gradient;
    gradient.layout = _gradient_pipeline_layout;
    gradient.name   = "gradient";
    gradient.data   = {};

    // default colors
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

    VK_CHECK(vkCreateComputePipelines(
        _device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &gradient.pipeline));

    // change shader module to create sky shader
    compute_pipeline_create_info.stage.module = sky_shader;

    ComputeEffect sky;
    sky.layout = _gradient_pipeline_layout;
    sky.name   = "sky";
    sky.data   = {};
    // default sky parameters
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

    VK_CHECK(
        vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &sky.pipeline));

    // add background effects to the array
    background_effects.push_back(gradient);
    background_effects.push_back(sky);

    // destroy structures
    vkDestroyShaderModule(_device, gradient_shader, nullptr);
    vkDestroyShaderModule(_device, sky_shader, nullptr);
    _main_deletion_queue.push_function(
        [=]()
        {
            vkDestroyPipelineLayout(_device, _gradient_pipeline_layout, nullptr);
            vkDestroyPipeline(_device, sky.pipeline, nullptr);
            vkDestroyPipeline(_device, gradient.pipeline, nullptr);
        });
}

void VulkanEngine::init_mesh_pipeline()
{
    VkShaderModule triangle_frag_shader;
    if (!vkutil::load_shader_module("./Shaders/tex_image.frag.debug.spv", _device, &triangle_frag_shader))
    {
        fmt::print("Error when building the triangle fragment shader module");
    }
    else
    {
        fmt::print("Triangle fragment shader successfully loaded\n");
    }

    VkShaderModule triangle_vertex_shader;
    if (!vkutil::load_shader_module("./Shaders/colored_triangle_mesh.vert.debug.spv", _device, &triangle_vertex_shader))
    {
        fmt::print("Error when building the triangle vertex shader module");
    }
    else
    {
        fmt::print("Triangle vertex shader successfully loaded\n");
    }

    // Push constants
    VkPushConstantRange buffer_range{};
    buffer_range.offset     = 0;
    buffer_range.size       = sizeof(GPUDrawPushConstants);
    buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // pipeline layout to control input/output of the shader
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.pPushConstantRanges        = &buffer_range;
    pipeline_layout_info.pushConstantRangeCount     = 1;
    pipeline_layout_info.pSetLayouts                = &_single_image_descriptor_layout;
    pipeline_layout_info.setLayoutCount             = 1;
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_mesh_pipeline_layout));

    PipelineBuilder pipeline_builder;
    pipeline_builder._pipeline_layout = _mesh_pipeline_layout;
    pipeline_builder.set_shaders(triangle_vertex_shader, triangle_frag_shader);
    pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipeline_builder.set_multisampling_none();
    // pipeline_builder.disable_blending();
    // pipeline_builder.enable_blending_additive();
    pipeline_builder.enable_blending_alphablend();
    // pipeline_builder.disable_depthtest();
    pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    pipeline_builder.set_color_attachment_format(_draw_image.image_format);
    pipeline_builder.set_depth_format(_depth_image.image_format);

    _mesh_pipeline = pipeline_builder.build_pipeline(_device);

    // clean structs
    vkDestroyShaderModule(_device, triangle_frag_shader, nullptr);
    vkDestroyShaderModule(_device, triangle_vertex_shader, nullptr);

    _main_deletion_queue.push_function(
        [&]()
        {
            vkDestroyPipelineLayout(_device, _mesh_pipeline_layout, nullptr);
            vkDestroyPipeline(_device, _mesh_pipeline, nullptr);
        });
}

void VulkanEngine::init_descriptors()
{
    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    };

    global_descriptor_allocator.init(_device, 10, sizes);

    _main_deletion_queue.push_function([&]() { global_descriptor_allocator.destroy_pools(_device); });

    // make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _draw_image_descriptor_layout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    // descriptor set layout for single image
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _single_image_descriptor_layout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    // descriptor set layout for scene
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpu_scene_data_descriptor_layout =
            builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    _main_deletion_queue.push_function(
        [&]()
        {
            vkDestroyDescriptorSetLayout(_device, _draw_image_descriptor_layout, nullptr);
            vkDestroyDescriptorSetLayout(_device, _single_image_descriptor_layout, nullptr);
            vkDestroyDescriptorSetLayout(_device, _gpu_scene_data_descriptor_layout, nullptr);
        });

    // allocate a descriptor set for our draw image
    _draw_image_descriptors = global_descriptor_allocator.allocate(_device, _draw_image_descriptor_layout);
    {
        DescriptorWriter writer;
        writer.write_image(
            0, _draw_image.image_view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        writer.update_set(_device, _draw_image_descriptors);
    }

    // Frame descriptors

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        // create descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            {         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            {        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        _frames[i]._frame_descriptors = DescriptorAllocatorGrowable{};
        _frames[i]._frame_descriptors.init(_device, 1000, frame_sizes);

        _main_deletion_queue.push_function([&, i]() { _frames[i]._frame_descriptors.destroy_pools(_device); });
    }
}

AllocatedBuffer VulkanEngine::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.pNext              = nullptr;
    buffer_info.size               = alloc_size;

    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_alloc_info = {};
    vma_alloc_info.usage                   = memory_usage;
    vma_alloc_info.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer new_buffer;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(
        _allocator, &buffer_info, &vma_alloc_info, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

    return new_buffer;
}

GPUMeshBuffers VulkanEngine::upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size  = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers new_surface;

    // create vertex buffer
    new_surface.vertex_buffer = create_buffer(vertex_buffer_size,
                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY);

    // find the adress of the vertex buffer
    VkBufferDeviceAddressInfo device_address_info{.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                  .buffer = new_surface.vertex_buffer.buffer};
    new_surface.vertex_buffer_address = vkGetBufferDeviceAddress(_device, &device_address_info);

    // create index buffer
    new_surface.index_buffer = create_buffer(index_buffer_size,
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(
        vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();

    // copy vertex buffer
    memcpy(data, vertices.data(), vertex_buffer_size);
    // copy index buffer
    memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

    immediate_submit(
        [&](VkCommandBuffer cmd)
        {
            VkBufferCopy vertex_copy{0};
            vertex_copy.dstOffset = 0;
            vertex_copy.srcOffset = 0;
            vertex_copy.size      = vertex_buffer_size;

            vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertex_copy);

            VkBufferCopy index_copy{0};
            index_copy.dstOffset = 0;
            index_copy.srcOffset = vertex_buffer_size;
            index_copy.size      = index_buffer_size;

            vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &index_copy);
        });

    destroy_buffer(staging);

    return new_surface;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage VulkanEngine::create_image(VkExtent3D size,
                                          VkFormat format,
                                          VkImageUsageFlags usage,
                                          bool mipmapped /*= false*/)
{
    AllocatedImage new_image;
    new_image.image_format = format;
    new_image.image_extent = size;

    VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
    if (mipmapped)
    {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &new_image.image, &new_image.allocation, nullptr));

    // set aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT)
    {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo view_info       = vkinit::imageview_create_info(format, new_image.image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &new_image.image_view));

    return new_image;
}

AllocatedImage VulkanEngine::create_image(void* data,
                                          VkExtent3D size,
                                          VkFormat format,
                                          VkImageUsageFlags usage,
                                          bool mipmapped /*= false*/)
{
    // create staging buffer to hold pixel data
    size_t data_size = size.depth * size.width * size.height * 4;
    AllocatedBuffer upload_buffer =
        create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(upload_buffer.info.pMappedData, data, data_size);

    AllocatedImage new_image = create_image(
        size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    // copy staging buffer pixel data into the new_image
    immediate_submit(
        [&](VkCommandBuffer cmd)
        {
            vkutil::transition_image(
                cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copy_region = {};
            copy_region.bufferOffset      = 0;
            copy_region.bufferRowLength   = 0;
            copy_region.bufferImageHeight = 0;

            copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy_region.imageSubresource.mipLevel       = 0;
            copy_region.imageSubresource.baseArrayLayer = 0;
            copy_region.imageSubresource.layerCount     = 1;
            copy_region.imageExtent                     = size;

            vkCmdCopyBufferToImage(
                cmd, upload_buffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            vkutil::transition_image(
                cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

    destroy_buffer(upload_buffer);

    return new_image;
}

void VulkanEngine::destroy_image(const AllocatedImage& image)
{
    vkDestroyImageView(_device, image.image_view, nullptr);
    vmaDestroyImage(_allocator, image.image, image.allocation);
}

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
{
    VkShaderModule mesh_frag_shader;
    if (!vkutil::load_shader_module("./Shaders/mesh.frag.debug.spv", engine->_device, &mesh_frag_shader))
    {
        fmt::println("Error when building the mesh fragment shader module");
    }

    VkShaderModule mesh_vertex_shader;
    if (!vkutil::load_shader_module("./Shaders/mesh.vert.debug.spv", engine->_device, &mesh_vertex_shader))
    {
        fmt::println("Error when building the mesh vertex shader module");
    }

    VkPushConstantRange matrix_range{};
    matrix_range.offset     = 0;
    matrix_range.size       = sizeof(GPUDrawPushConstants);
    matrix_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layout_builder;
    layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layout_builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layout_builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    material_layout = layout_builder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {engine->_gpu_scene_data_descriptor_layout, material_layout};

    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount             = 2;
    mesh_layout_info.pSetLayouts                = layouts;
    mesh_layout_info.pPushConstantRanges        = &matrix_range;
    mesh_layout_info.pushConstantRangeCount     = 1;

    VkPipelineLayout new_layout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &new_layout));

    opaque_pipeline.layout      = new_layout;
    transparent_pipeline.layout = new_layout;

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    PipelineBuilder pipeline_builder;
    pipeline_builder.set_shaders(mesh_vertex_shader, mesh_frag_shader);
    pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipeline_builder.set_multisampling_none();
    pipeline_builder.disable_blending();
    pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipeline_builder.set_color_attachment_format(engine->_draw_image.image_format);
    pipeline_builder.set_depth_format(engine->_depth_image.image_format);

    // use the triangle layout we created
    pipeline_builder._pipeline_layout = new_layout;

    // finally build the pipeline
    opaque_pipeline.pipeline = pipeline_builder.build_pipeline(engine->_device);

    // create the transparent variant
    pipeline_builder.enable_blending_additive();

    pipeline_builder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    transparent_pipeline.pipeline = pipeline_builder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, mesh_frag_shader, nullptr);
    vkDestroyShaderModule(engine->_device, mesh_vertex_shader, nullptr);
}

void GLTFMetallic_Roughness::clear_resources(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, material_layout, nullptr);
    vkDestroyPipelineLayout(device, transparent_pipeline.layout, nullptr);

    vkDestroyPipeline(device, transparent_pipeline.pipeline, nullptr);
    vkDestroyPipeline(device, opaque_pipeline.pipeline, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device,
                                                        MaterialPass pass,
                                                        const MaterialResources& resources,
                                                        DescriptorAllocatorGrowable& descriptor_allocator)
{
    MaterialInstance mat_data;
    mat_data.pass_type = pass;
    if (pass == MaterialPass::Transparent)
    {
        mat_data.pipeline = &transparent_pipeline;
    }
    else
    {
        mat_data.pipeline = &opaque_pipeline;
    }

    mat_data.material_set = descriptor_allocator.allocate(device, material_layout);

    writer.clear();
    writer.write_buffer(0,
                        resources.data_buffer,
                        sizeof(MaterialConstants),
                        resources.data_buffer_offset,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1,
                       resources.color_image.image_view,
                       resources.color_sampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(2,
                       resources.metal_rough_image.image_view,
                       resources.metal_rough_sampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.update_set(device, mat_data.material_set);

    return mat_data;
}

// HACK: Returns early if mesh is a nullptr
// FIXME: The guide does NOT do this, find another way to fix this later.

void MeshNode::draw(const glm::mat4& top_matrix, DrawContext& ctx)
{
    glm::mat4 node_matrix = top_matrix * world_transform;

    for (auto& s : mesh->surfaces)
    {
        RenderObject def;
        def.index_count           = s.count;
        def.first_index           = s.start_index;
        def.index_buffer          = mesh->mesh_buffers.index_buffer.buffer;
        def.material              = &s.material->data;
        def.bounds                = s.bounds;
        def.transform             = node_matrix;
        def.vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address;

        if (s.material->data.pass_type == MaterialPass::Transparent)
        {
            ctx.transparent_surfaces.push_back(def);
        }
        else
        {
            ctx.opaque_surfaces.push_back(def);
        }
    }

    // recurse down
    Node::draw(top_matrix, ctx);
}