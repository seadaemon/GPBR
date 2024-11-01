//==> INCLUDES
#define VOLK_IMPLEMENTATION
#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <gpbr/Graphics/Vulkan/vk_initializers.h>
#include <gpbr/Graphics/Vulkan/vk_types.h>

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

    vkb::Instance vkb_inst   = inst_ret.value();
    _instance                = vkb_inst.instance;
    _debug_messenger         = vkb_inst.debug_messenger;
    _instance_dispatch_table = vkb_inst.make_table();
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
    _device         = vkb_device.device;
    _chosen_GPU     = physical_device.physical_device;
    _dispatch_table = vkb_device.make_table();
    //<== INIT DEVICE

    //==> LOAD VULKAN ENTRYPOINTS
    volkInitialize();
    volkLoadInstance(_instance);
    //<== LOAD VULKAN ENTRYPOINTS

    //==> INIT QUEUE
    // get graphics queue
    //_graphics_queue
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

void VulkanEngine::init_commands() {}

void VulkanEngine::init_sync_structures() {}

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
        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vkDestroyDevice(_device, nullptr);

        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);

        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::draw() {}

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