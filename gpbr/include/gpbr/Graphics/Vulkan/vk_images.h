/* vk_images.h
 *
 * Provides methods to manipulate and process Vulkan images.
 *
 */
#pragma once

#include <vulkan/vulkan.h>

namespace vkutil
{
// Performs a layout transition for a given image.
void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

// Copies an image using VkCmdBlitImage.
void copy_image_to_image(VkCommandBuffer cmd,
                         VkImage source,
                         VkImage destination,
                         VkExtent2D srcSize,
                         VkExtent2D dstSize);

// Creates mipmaps for a given image.
void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size);
} // namespace vkutil