#include <chrono>
#include <thread>

#include "gpbr/Graphics/Vulkan/vk_engine.h"

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

int main(int argc, char* argv[])
{
    VulkanEngine engine;

    engine.init();

    TimePoint start_time = std::chrono::high_resolution_clock::now();

    engine.run();

    TimePoint end_time = std::chrono::high_resolution_clock::now();

    engine.cleanup();

    std::chrono::duration<double, std::milli> elapsed_time = end_time - start_time;

    fmt::print("Ran for {:.3f}ms", elapsed_time.count());

    return 0;
}