#pragma once

// class DescriptorManager {
// public:
//     DescriptorManager(VkDevice device);
//     ~DescriptorManager();
//
//     // Create a layout with arbitrary bindings
//     VkDescriptorSetLayout createLayout(
//         const std::vector<VkDescriptorSetLayoutBinding>& bindings,
//         const std::vector<VkDescriptorBindingFlags>& bindingFlags = {}
//     );
//
//     // Create a pool for a given set of sizes
//     VkDescriptorPool createPool(
//         const std::vector<VkDescriptorPoolSize>& poolSizes,
//         uint32_t maxSets,
//         VkDescriptorPoolCreateFlags flags = 0
//     );
//
//     // Allocate a descriptor set
//     VkDescriptorSet allocateSet(
//         VkDescriptorPool pool,
//         VkDescriptorSetLayout layout,
//         uint32_t variableCount = 0
//     );
//
//     // Update descriptors
//     void updateImage(
//         VkDescriptorSet set,
//         uint32_t binding,
//         uint32_t arrayElement,
//         VkImageView view,
//         VkSampler sampler,
//         VkImageLayout layout
//     );
//
//     void updateBuffer(
//         VkDescriptorSet set,
//         uint32_t binding,
//         VkBuffer buffer,
//         VkDeviceSize range,
//         VkDescriptorType type
//     );
//
// private:
//     VkDevice m_device;
// };
