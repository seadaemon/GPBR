/*
 * Provides functionality to create and manipulate descriptors.
 * This includes handling memory allocation (via pools), creating layouts,
 * creating bindings, and creating/updating descriptor sets.
 */
#pragma once

#include <vector>
#include "vk_types.h"
#include <deque>
#include <span>

// Manages the creation of a descriptor set layout.
struct DescriptorLayoutBuilder
{
    // List of bindings to be used when building the layout.
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    // Creates a binding of the given type and adds it to the list.
    void add_binding(uint32_t binding_num, VkDescriptorType type);
    // Removes all bindings.
    void clear();
    // Returns a VkDescriptorSetLayout with the current bindings.
    VkDescriptorSetLayout build(VkDevice device,
                                VkShaderStageFlags shader_stages,
                                void* p_next                           = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
};

// Manages the creation and execution of write operations for a descriptor set.
struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> image_infos;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
    // Creates a write operation for a VkImageView object.
    void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    // Creates a write operation for a VkBuffer object.
    void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    // Clears all internal data structures.
    void clear();
    // Updates the contents of the given descriptor set.
    void update_set(VkDevice device, VkDescriptorSet set);
};

// Manages the memory of a single descriptor pool.
struct [[deprecated("Use DescriptorAllocatorGrowable instead.")]] DescriptorAllocator
{
    // Contains a descriptor type and a ratio used to calculate the total number of descriptors that can be allocated
    // (across all descriptor sets) in a given pool.
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;
    // Creates a new descriptor pool.
    void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    // Resets the descriptor pool.
    void clear_descriptors(VkDevice device);
    // Destroys the descriptor pool.
    void destroy_pool(VkDevice device);
    // Allocates a descriptor set to the pool.
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

// Allocates memory for multiple VkDescriptorPool objects.
struct DescriptorAllocatorGrowable
{
  public:
    // Contains a descriptor type and a ratio used to calculate the total number of descriptors that can be allocated
    // (across all descriptor sets) in a given pool.
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };
    // Allocates the first descriptor pool and adds it to the allocator's ready_pools array.
    void init(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    // Resets all descriptor pools and places them in the ready_pools list.
    void clear_pools(VkDevice device);
    // Destroys all descriptor pools.
    void destroy_pools(VkDevice device);
    // Creates and returns a VkDescriptorSet object using a specified layout.
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* p_next = nullptr);

  private:
    // Returns an existing pool if one is ready, or creates a new pool if none are available. If a new pool is created,
    // the number of sets-per-pool is increased.
    VkDescriptorPool get_pool(VkDevice device);
    // Creates a VkDescriptorPool with a specified number of descriptor sets and a specified number of descriptors.
    VkDescriptorPool create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);
    // List of PoolSizeRatio objects.
    std::vector<PoolSizeRatio> ratios;
    // List of fully allocated pools.
    std::vector<VkDescriptorPool> full_pools;
    // List of new and/or usable pools.
    std::vector<VkDescriptorPool> ready_pools;
    uint32_t sets_per_pool;
};