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

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
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
    // the application must only have one engine
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    // initialize SDL & create a window
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        // TODO add error handling here
        fmt::println("Could not initialize SDL: {}", SDL_GetError());
        return;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow("George's Physically Based Renderer", //
                               _window_extent.width,                 //
                               _window_extent.height,                //
                               window_flags                          //
    );

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    init_descriptors();

    init_pipelines();

    init_imgui();

    init_default_data();

    _is_initialized = true;
}

void VulkanEngine::init_default_data()
{
    test_meshes = load_gltf_meshes(this, ".\\Assets\\basicmesh.glb").value();
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

void VulkanEngine::cleanup()
{
    if (_is_initialized)
    {
        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(_device, _frames[i]._command_pool, nullptr);

            // destroy sync objects
            vkDestroyFence(_device, _frames[i]._render_fence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._swapchain_semaphore, nullptr);

            _frames[i]._deletion_queue.flush();
        }

        for (auto& mesh : test_meshes)
        {
            destroy_buffer(mesh->mesh_buffers.index_buffer);
            destroy_buffer(mesh->mesh_buffers.vertex_buffer);
        }

        _main_deletion_queue.flush();

        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
    // make clear-color | flash blue with a 120 frame period
    VkClearColorValue clear_value;
    float flash = std::abs(std::sin(_frame_number / 120.0f));
    clear_value = {
        {0.0f, 0.0f, flash, 1.0f}
    };

    VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

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

void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
    // begin a render pass  connected to draw_image
    VkRenderingAttachmentInfo color_attachment =
        vkinit::attachment_info(_draw_image.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depth_attachment =
        vkinit::attachment_info(_depth_image.imageView, nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo render_info = vkinit::rendering_info(_window_extent, &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline);

    // set dynamic viewport and scissor
    VkViewport viewport = {};
    viewport.x          = 0;
    viewport.y          = 0;
    viewport.width      = _draw_extent.width;
    viewport.height     = _draw_extent.height;
    viewport.minDepth   = 0.f;
    viewport.maxDepth   = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor      = {};
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = viewport.width;
    scissor.extent.height = viewport.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    GPUDrawPushConstants push_constants;

    // loading a 3D mesh
    glm::mat4 view = glm::translate(glm::vec3{0, 0, -5}); // 5 units from origin
    view           = glm::rotate(view, glm::radians(45.0f), glm::vec3(0, 1, 0));

    //  note "near" is 1000 and "far" is 0.1 because: near plane: depth 1 | far plane: depth 0
    //  this is done to increase quality of depth testing (to be seen later)
    glm::mat4 projection =
        glm::perspective(glm::radians(70.0f), (float)_draw_extent.width / (float)_draw_extent.height, 1000.0f, 0.1f);

    // invert Y direction to align with OpenGL/glTF axis
    projection[1][1] *= -1;

    // projection * view * model
    push_constants.world_matrix = projection * view;

    uint32_t mesh_idx = 2;

    push_constants.vertex_buffer = test_meshes[mesh_idx]->mesh_buffers.vertex_buffer_address;

    vkCmdPushConstants(
        cmd, _mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
    vkCmdBindIndexBuffer(cmd, test_meshes[mesh_idx]->mesh_buffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        cmd, test_meshes[mesh_idx]->surfaces[0].count, 1, test_meshes[mesh_idx]->surfaces[0].start_index, 0, 0);

    vkCmdEndRendering(cmd);
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

void VulkanEngine::draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, true, 1000000000));

    get_current_frame()._deletion_queue.flush();

    // request image from the swapchain
    uint32_t swapchain_image_index;

    VK_CHECK(vkAcquireNextImageKHR(
        _device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index));

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._render_fence));

    // reset command buffer to begin recording again
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._main_command_buffer, 0));

    // naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._main_command_buffer;

    // begin recording. This command buffer is used exactly once
    VkCommandBufferBeginInfo cmd_begin_info =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    _draw_extent.width  = _draw_image.imageExtent.width;
    _draw_extent.height = _draw_image.imageExtent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // transition the main draw image into general layout so it can be written to
    vkutil::transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    vkutil::transition_image(cmd, _draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::transition_image(
        cmd, _depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    draw_geometry(cmd);

    // transition the draw image and the swapchain image into their correct transfer layouts
    vkutil::transition_image(
        cmd, _draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(
        cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy from draw image into the swapchain
    vkutil::copy_image_to_image(
        cmd, _draw_image.image, _swapchain_images[swapchain_image_index], _draw_extent, _swapchain_extent);

    // set swapchain image layout to Attachment Optimal for drawing
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

    // finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

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

    VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

    _frame_number++;
}

//==> MAIN LOOP
void VulkanEngine::run()
{
    SDL_Event e;
    bool quit = false;

    // main loop
    while (!quit)
    {
        // Poll until all events are handled
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            //==> WINDOW EVENTS
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                _stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED)
            {
                _stop_rendering = false;
            }
            //<== WINDOW EVENTS

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

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("background"))
        {
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

        draw();
    }
}
//<== MAIN LOOP

void VulkanEngine::init_vulkan()
{
    //==> INIT INSTANCE
    vkb::InstanceBuilder builder;

    // vulkan instance w/ debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(use_validation_layers) //
                        .use_default_debug_messenger()                    //
                        .require_api_version(1, 3, 0)                     //
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    _instance              = vkb_inst.instance;
    _debug_messenger       = vkb_inst.debug_messenger;
    //<== INIT INSTANCE

    //==> INIT DEVICE
    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    // vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing  = true;

    // select a gpu
    vkb::PhysicalDeviceSelector selector{vkb_inst};

    selector.add_required_extension("VK_KHR_dynamic_rendering");

    vkb::PhysicalDevice physical_device = selector                                  //
                                              .set_minimum_version(1, 3)            //
                                              .set_required_features_13(features)   //
                                              .set_required_features_12(features12) //
                                              .set_surface(_surface)                //
                                              .select()                             //
                                              .value();

    // create the vulkan device
    vkb::DeviceBuilder device_builder{physical_device};

    vkb::Device vkb_device = device_builder.build().value();

    // get device handle;
    _device     = vkb_device.device;
    _chosen_GPU = physical_device.physical_device;
    //<== INIT DEVICE

    //==> INIT VOLK
    // load vulkan function entrypoints using VOLK
    volkInitialize();
    volkLoadInstance(_instance);
    volkLoadDevice(_device);
    //<== INIT VOLK

    // get a graphics queue using vkbootstrap
    _graphics_queue        = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // gather Vulkan functions for VMA
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

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.pVulkanFunctions       = &vma_funcs;
    allocator_info.physicalDevice         = _chosen_GPU;
    allocator_info.device                 = _device;
    allocator_info.instance               = _instance;
    allocator_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocator_info, &_allocator);

    _main_deletion_queue.push_function([&]() { vmaDestroyAllocator(_allocator); });
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{_chosen_GPU, _device, _surface};

    _swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain =
        swapchain_builder
            .set_desired_format(VkSurfaceFormatKHR{.format     = _swapchain_image_format,            //
                                                   .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}) //
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)     // present mode enables vsync
            .set_desired_extent(width, height)                      //
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT) //
            .build()                                                //
            .value();

    _swapchain_extent = vkb_swapchain.extent;
    // store swapchain & related images
    _swapchain             = vkb_swapchain.swapchain;
    _swapchain_images      = vkb_swapchain.get_images().value();
    _swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain()
{
    create_swapchain(_window_extent.width, _window_extent.height);

    //==> INIT DRAW_IMAGE
    VkExtent3D draw_image_extent = {
        _window_extent.width,  //
        _window_extent.height, //
        1                      //
    };

    // hardcode the draw format to a 32 bit signed float
    _draw_image.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _draw_image.imageExtent = draw_image_extent;

    VkImageUsageFlags draw_image_usages{};
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info =
        vkinit::image_create_info(_draw_image.imageFormat, draw_image_usages, draw_image_extent);

    // for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image | 'r' prefix stands for "render"
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_draw_image.image, &_draw_image.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info =
        vkinit::imageview_create_info(_draw_image.imageFormat, _draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_draw_image.imageView));
    //<== END INIT DRAW_IMAGE

    //==> INIT DEPTH_IMAGE
    _depth_image.imageFormat = VK_FORMAT_D32_SFLOAT;
    _depth_image.imageExtent = draw_image_extent;
    VkImageUsageFlags depth_image_usages{};
    depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info =
        vkinit::image_create_info(_depth_image.imageFormat, depth_image_usages, draw_image_extent);

    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depth_image.image, &_depth_image.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info =
        vkinit::imageview_create_info(_depth_image.imageFormat, _depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depth_image.imageView));
    //<== END INIT DEPTH_IMAGE

    _main_deletion_queue.push_function(
        [=]()
        {
            vkDestroyImageView(_device, _draw_image.imageView, nullptr);
            vmaDestroyImage(_allocator, _draw_image.image, _draw_image.allocation);

            vkDestroyImageView(_device, _depth_image.imageView, nullptr);
            vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation);
        });
}

void VulkanEngine::init_commands()
{
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_info =
        vkinit::command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[i]._command_pool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(_frames[i]._command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_frames[i]._main_command_buffer));
    }

    // immediate commands
    VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_imm_command_pool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(_imm_command_pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_imm_command_buffer));

    _main_deletion_queue.push_function([=]() { vkDestroyCommandPool(_device, _imm_command_pool, nullptr); });
}

void VulkanEngine::init_sync_structures()
{
    // create syncronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain
    // we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fence_create_info         = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));
    }

    // immediate fence
    VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_imm_fence));
    _main_deletion_queue.push_function([=]() { vkDestroyFence(_device, _imm_fence, nullptr); });
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

    // add the destroy the imgui created structures
    //*
    _main_deletion_queue.push_function(
        [=]()
        {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(_device, imgui_pool, nullptr);
        });
    //*/
}

void VulkanEngine::init_pipelines()
{
    // COMPUTE PIPELINES
    init_background_pipelines();

    // GRAPHICS PIPELINES
    init_mesh_pipeline();
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
    if (!vkutil::load_shader_module("./Shaders/colored_triangle.frag.spv", _device, &triangle_frag_shader))
    {
        fmt::print("Error when building the triangle fragment shader module");
    }
    else
    {
        fmt::print("Triangle fragment shader successfully loaded\n");
    }

    VkShaderModule triangle_vertex_shader;
    if (!vkutil::load_shader_module("./Shaders/colored_triangle_mesh.vert.spv", _device, &triangle_vertex_shader))
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

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_mesh_pipeline_layout));

    PipelineBuilder pipeline_builder;
    pipeline_builder._pipeline_layout = _mesh_pipeline_layout;
    pipeline_builder.set_shaders(triangle_vertex_shader, triangle_frag_shader);
    pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipeline_builder.set_multisampling_none();
    pipeline_builder.disable_blending();
    // pipeline_builder.disable_depthtest();
    pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    pipeline_builder.set_color_attachment_format(_draw_image.imageFormat);
    pipeline_builder.set_depth_format(_depth_image.imageFormat);

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
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    global_descriptor_allocator.init_pool(_device, 10, sizes);

    // make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _draw_image_descriptor_layout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // allocate a descriptor set for our draw image
    _draw_image_descriptors = global_descriptor_allocator.allocate(_device, _draw_image_descriptor_layout);

    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_info.imageView   = _draw_image.imageView;

    VkWriteDescriptorSet draw_image_write = {};
    draw_image_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    draw_image_write.pNext                = nullptr;

    draw_image_write.dstBinding      = 0;
    draw_image_write.dstSet          = _draw_image_descriptors;
    draw_image_write.descriptorCount = 1;
    draw_image_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    draw_image_write.pImageInfo      = &img_info;

    vkUpdateDescriptorSets(_device, 1, &draw_image_write, 0, nullptr);

    // make sure both the descriptor allocator and the new layout get cleaned up properly
    _main_deletion_queue.push_function(
        [&]()
        {
            global_descriptor_allocator.destroy_pool(_device);

            vkDestroyDescriptorSetLayout(_device, _draw_image_descriptor_layout, nullptr);
        });
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