#pragma once

// #include "volk_impl.h"
#include <vulkan/vulkan.h>

namespace vkutil
{

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);

} // namespace vkutil
