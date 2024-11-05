//==> INCLUDES
#define VOLK_IMPLEMENTATION
#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <gpbr/Graphics/Vulkan/vk_initializers.h>
#include <gpbr/Graphics/Vulkan/vk_types.h>
#include <gpbr/Graphics/Vulkan/vk_images.h>

#include "VkBootstrap.h"
#include <array>
#include <chrono>
#include <thread>
//<== INCLUDES

//==> INIT
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

    _is_initialized = true;
}
//<== INIT

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
        }

        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::draw()
{
    // wait until the gpu has finished rendering the last frame
    // 1 second timeout
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._render_fence));

    // request image from the swapchain
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(
        _device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index));

    // naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._main_command_buffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know
    // that
    VkCommandBufferBeginInfo cmd_begin_info =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // make the swapchain image into writeable mode before rendering
    vkutil::transition_image(
        cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clear_value;
    float flash = std::abs(std::sin(_frame_number / 120.f));
    clear_value = {
        {0.0f, 0.0f, flash, 1.0f}
    };

    VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    // clear image
    vkCmdClearColorImage(
        cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

    // make the swapchain image into presentable mode
    vkutil::transition_image(
        cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

    // increase the number of frames drawn
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

    //==> LOAD VULKAN ENTRYPOINTS
    volkInitialize();
    volkLoadInstance(_instance);
    //<== LOAD VULKAN ENTRYPOINTS

    //==> INIT QUEUE
    _graphics_queue        = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    //<== INIT QUEUE
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
}