// vk_initializers.h
// abstracts initialization of Vulkan structures
#pragma once

#include "vk_types.h"

namespace vkinit
{
VkCommandPoolCreateInfo command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);
VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

VkQueryPoolCreateInfo query_pool_create_info(VkQueryType query_type,
                                             uint32_t count               = 1,
                                             VkQueryPoolCreateFlags flags = 0);

VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd,
                          VkSemaphoreSubmitInfo* signal_semaphore_info,
                          VkSemaphoreSubmitInfo* wait_semaphore_info);
VkPresentInfoKHR present_info();

VkRenderingAttachmentInfo attachment_info(VkImageView view,
                                          VkClearValue* clear,
                                          VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo depth_attachment_info(VkImageView view,
                                                VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo rendering_info(VkExtent2D render_extent,
                               VkRenderingAttachmentInfo* color_attachment,
                               VkRenderingAttachmentInfo* depth_attachment);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type,
                                                          VkShaderStageFlags stage_flags,
                                                          uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(VkDescriptorSetLayoutBinding* bindings,
                                                                 uint32_t binding_count);
VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type,
                                            VkDescriptorSet dst_set,
                                            VkDescriptorImageInfo* image_info,
                                            uint32_t binding);
VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
                                             VkDescriptorSet dst_set,
                                             VkDescriptorBufferInfo* buffer_info,
                                             uint32_t binding);
VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);
VkPipelineLayoutCreateInfo pipeline_layout_create_info();
VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                                                  VkShaderModule shader_module,
                                                                  const char* entry = "main");
} // namespace vkinit