/* Contains
 *
 */
#pragma once
#include <vector>
#include "vk_types.h"
#include <deque>
#include <span>

// TBA
struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    // TBA
    void add_binding(uint32_t binding, VkDescriptorType type);
    // TBA
    void clear();
    // TBA
    VkDescriptorSetLayout build(VkDevice device,
                                VkShaderStageFlags shader_stages,
                                void* p_next                           = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
};

// TBA
struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> image_infos;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
    // TBA
    void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    // TBA
    void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    // TBA
    void clear();
    // TBA
    void update_set(VkDevice device, VkDescriptorSet set);
};

// TBA
struct DescriptorAllocator
{
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;
    // TBA
    void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    // TBA
    void clear_descriptors(VkDevice device);
    // TBA
    void destroy_pool(VkDevice device);
    // TBA
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

// TBA
struct DescriptorAllocatorGrowable
{
  public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };
    // TBA
    void init(VkDevice device, uint32_t initial_sets, std::span<PoolSizeRatio> pool_ratios);
    // TBA
    void clear_pools(VkDevice device);
    void destroy_pools(VkDevice device);
    // TBA
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* p_next = nullptr);

  private:
    // TBA
    VkDescriptorPool get_pool(VkDevice device);
    // TBA
    VkDescriptorPool create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> full_pools;
    std::vector<VkDescriptorPool> ready_pools;
    uint32_t sets_per_pool;
};