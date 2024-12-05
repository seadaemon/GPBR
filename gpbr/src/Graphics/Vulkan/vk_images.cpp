#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_images.h"
#include "gpbr/Graphics/Vulkan/vk_initializers.h"

void vkutil::transition_image(VkCommandBuffer cmd,
                              VkImage image,
                              VkImageLayout current_layout,
                              VkImageLayout new_layout)
{
    VkImageMemoryBarrier2 image_barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_barrier.pNext = nullptr;

    // Todo: use more appropriate stage masks instead of using ALL_COMMANDS
    image_barrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    image_barrier.oldLayout = current_layout;
    image_barrier.newLayout = new_layout;

    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
                                         VK_IMAGE_ASPECT_DEPTH_BIT :
                                         VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange = vkinit::image_subresource_range(aspect_mask);
    image_barrier.image            = image;

    VkDependencyInfo dep_info{};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.pNext = nullptr;

    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &image_barrier;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

void vkutil::copy_image_to_image(VkCommandBuffer cmd,
                                 VkImage source,
                                 VkImage destination,
                                 VkExtent2D srcSize,
                                 VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.dstSubresource.mipLevel       = 0;

    VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
    blitInfo.dstImage       = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage       = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter         = VK_FILTER_LINEAR;
    blitInfo.regionCount    = 1;
    blitInfo.pRegions       = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

void vkutil::generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size)
{
    int mipLevels = int(std::floor(std::log2(std::max(image_size.width, image_size.height)))) + 1;
    for (int mip = 0; mip < mipLevels; mip++)
    {

        VkExtent2D half_size = image_size;
        half_size.width /= 2;
        half_size.height /= 2;

        VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr};

        imageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange              = vkinit::image_subresource_range(aspectMask);
        imageBarrier.subresourceRange.levelCount   = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image                         = image;

        VkDependencyInfo depInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr};
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1)
        {
            VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

            blitRegion.srcOffsets[1].x = image_size.width;
            blitRegion.srcOffsets[1].y = image_size.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = half_size.width;
            blitRegion.dstOffsets[1].y = half_size.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount     = 1;
            blitRegion.srcSubresource.mipLevel       = mip;

            blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount     = 1;
            blitRegion.dstSubresource.mipLevel       = mip + 1;

            VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
            blitInfo.dstImage       = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage       = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter         = VK_FILTER_LINEAR;
            blitInfo.regionCount    = 1;
            blitInfo.pRegions       = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            image_size = half_size;
        }
    }

    // transition all mip levels into the final layout
    transition_image(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}