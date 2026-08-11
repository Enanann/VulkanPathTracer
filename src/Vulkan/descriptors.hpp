#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

/* 
Descriptor is a way for shaders to freely access resources (buffers/images)

Descriptor include 3 components:
    1. Descriptor set layout
    2. Descriptor pool
    3. Descriptor set

Descriptor set layout describes the structure of a descriptor set. It specifies what types of resources will be
available to the shader and which binding number they correspond to 
You can reuse descriptor set layout if multiple pipelines use the same layout

Descriptor pool is used to allocate memory for descriptor sets

Descriptor set specifies the actual buffer/image resources that will be bound to the descriptors,
it will then be bound for the drawing commands
*/
namespace poki {

// Descriptor Bindings
// Helps create descriptor set layout by storing each binding's information
// Does not store the descriptor set layout itself
class DescriptorBindings {
public:
    // Adds a binding at the index `binding` for `descriptorCount` descriptors of type `descriptorType`
    // The resources can be access at `stageFlags`
    void addBinding(uint32_t binding, 
                    vk::DescriptorType descriptorType, 
                    uint32_t descriptorCount, 
                    vk::ShaderStageFlags stageFlags, 
                    const vk::Sampler* pImmutableSamplers = nullptr);

    void addBinding(const vk::DescriptorSetLayoutBinding& layoutBinding);

    // Generates the descriptor layout corresponding to the added bindings
    [[nodiscard]] vk::raii::DescriptorSetLayout createDescriptorSetLayout(const vk::raii::Device& device, vk::DescriptorSetLayoutCreateFlags flags) const;

    // Fills a `vk::WriteDescriptorSet` for `descriptorCount` descriptors, starting at `dstArrayElement`
    // 
    // If `dstArrayElement = ~0U`, then we will create the write descriptor set for the entire binding
    // Meaning `dstArrayElement` is treated as 0, and `descriptorCount` is set to the original binding's descriptorCount
    //
    // Note: This is not fully filled yet, you will need to fill the image, buffer yourself or use `WriteSetContainer::append()`
    vk::WriteDescriptorSet getWriteSet(uint32_t                       binding,
                                       const vk::raii::DescriptorSet& dstSet,
                                       uint32_t                       dstArrayElement = ~0U,
                                       uint32_t                       descriptorCount = 1) const;

    // Returns the required pool sizes for `numSets` sets
    std::vector<vk::DescriptorPoolSize> calculatePoolSizes(uint32_t numSets = 1) const;

    // Return the added bindings
    const std::vector<vk::DescriptorSetLayoutBinding> getBindings() const {return m_bindings;}

private:
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    // Look up for m_bindings, map the binding number to the index of the above array
    /*
    m_bindings       = {
        {.binding = 1}, // index 0 
        {.binding = 3}, // index 1
        {.binding = 6}  // index 2
    };
    m_bindingToIndex = {
        NO_BINDING_INDEX,
        0,                // binding 1 -> index 0
        NO_BINDING_INDEX,
        1,                // binding 3 -> index 1
        NO_BINDING_INDEX,
        NO_BINDING_INDEX,
        2,                // binding 6 -> index 2
    };

    // get binding 6
    auto& b = m_bindings[m_bindingToIndex[6]];
    */
    std::vector<uint32_t> m_bindingToIndex;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

// DescriptorSetContainer
// Create a single layout and `numSets` descriptor sets allocated using that layot
// Manage its own descriptor pool; all descriptor sets can be freed at once by resetting the pool
class DescriptorSetContainer {
public:
    DescriptorSetContainer() = default;

    // initialize `m_layout`, `m_pool`, and `m_sets`
    // If `numSets` = 0, only creates the layout 
    void init(const DescriptorBindings&          bindings, 
              const vk::raii::Device&            device,
              uint32_t                           numSets     = 1,
              vk::DescriptorSetLayoutCreateFlags layoutFlags = {},
              vk::DescriptorPoolCreateFlags      poolFlags   = {});

    // Wrapper to get a `vk::WriteDescriptorSet` for a descriptor set stored in `m_sets` if it's not empty
    // Note: `dstSetIndex` is an index into `m_sets`, not the descriptor set number (`set = N`) in the pipeline layout.
    // See `DescriptorBinding::getWriteSet()`
    vk::WriteDescriptorSet makeWriteSet(uint32_t binding, uint32_t dstSetIndex, uint32_t dstArrayElement = ~0U, uint32_t descriptorCount = 1) const {
        // Since we're not using push descriptors, m_sets must exist
        assert(!m_sets.empty() && "`m_sets` cannot be empty");
        return m_bindings.getWriteSet(binding, m_sets[dstSetIndex], dstArrayElement, descriptorCount);
    }

    const vk::raii::DescriptorSetLayout&        getLayout() const {return m_layout;}
    const std::vector<vk::raii::DescriptorSet>& getSets() const {return m_sets;} 
    const vk::raii::DescriptorSet&              getSet(uint32_t setIndex) const {return m_sets[setIndex];} 

private:
    DescriptorBindings                   m_bindings;
    vk::raii::DescriptorSetLayout        m_layout{nullptr};
    vk::raii::DescriptorPool             m_pool{nullptr};
    std::vector<vk::raii::DescriptorSet> m_sets;

    const vk::raii::Device* m_device{nullptr};
};

/////////////////////////////////////////////////////////////////////////////////////////////////

// WriteSetContainer
// Used to construct and store vk::WriteDescriptorSet along with their corresponding descriptor info
class WriteSetContainer {
public:
    // Single element (writeSet.descriptorCount must be 1)
    void append(const vk::WriteDescriptorSet& writeSet, const poki::Buffer& buffer, vk::DeviceSize offset = 0, vk::DeviceSize range = vk::WholeSize);
    void append(const vk::WriteDescriptorSet& writeSet, const poki::Image& image, std::optional<vk::ImageLayout> overrideLayout = std::nullopt);

    // TODO: Multiple element (writeSet.descriptorCount > 1)

    // Update the pointers if necessary, then return the vector to call `Device::updateDescriptorSets()`
    const std::vector<vk::WriteDescriptorSet>& data();

private:
    // vk::DescriptorBufferInfo and vk::DescriptorInfo are the same size, 
    // so we use an union to reduce storage overhead when `WriteSetContainer::data()` is called
    union BufferOrImageData {
        vk::DescriptorBufferInfo buffer;
        vk::DescriptorImageInfo  image;

        // Required to explicitly initialize the appropriate union member
        BufferOrImageData(vk::DescriptorBufferInfo& b) {buffer = b;}
        BufferOrImageData(vk::DescriptorImageInfo& i) {image = i;}
    };
    static_assert(sizeof(vk::DescriptorBufferInfo) == sizeof(vk::DescriptorImageInfo));

    std::vector<vk::WriteDescriptorSet> m_writeSets;
    std::vector<BufferOrImageData>      m_bufferOrImageDatas;

    // vk::WriteDescriptorSet stores pointers to elements in m_bufferOrImageDatas.
    // Since `WriteSetContainer::append()` may cause the vector to reallocate itself,
    // which will invalidate the pointers in `m_writeSets`
    // We will need to update/rebuild those before calling `Device::updateDescriptorSets()`
    bool m_needPointerUpdate = true;
};
    
}; // namespace poki
