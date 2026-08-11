#include "descriptors.hpp"
#include "log.hpp"

#include <cassert>
#include <unordered_map>

namespace poki {

constexpr uint32_t NO_BINDING_INDEX = ~0U;

void DescriptorBindings::addBinding(uint32_t binding, 
                    vk::DescriptorType descriptorType, 
                    uint32_t descriptorCount, 
                    vk::ShaderStageFlags stageFlags, 
                    const vk::Sampler* pImmutableSamplers /*= nullptr*/) 
{
    addBinding(vk::DescriptorSetLayoutBinding{
        .binding            = binding,
        .descriptorType     = descriptorType,
        .descriptorCount    = descriptorCount,
        .stageFlags         = stageFlags,
        .pImmutableSamplers = pImmutableSamplers
    });
}

void DescriptorBindings::addBinding(const vk::DescriptorSetLayoutBinding& layoutBinding) {
    m_bindings.push_back(layoutBinding);

    // update m_bindingToIndex
    uint32_t binding = layoutBinding.binding;
    if (m_bindingToIndex.size() <= binding) {
        m_bindingToIndex.resize(binding + 1, NO_BINDING_INDEX);
    }
    m_bindingToIndex[binding] = m_bindings.size() - 1;
}

[[nodiscard]] vk::raii::DescriptorSetLayout DescriptorBindings::createDescriptorSetLayout(const vk::raii::Device& device, vk::DescriptorSetLayoutCreateFlags flags) const {
    vk::DescriptorSetLayoutCreateInfo createInfo{
        .flags        = flags,
        .bindingCount = static_cast<uint32_t>(m_bindings.size()),
        .pBindings    = m_bindings.data()
    };
    return vk::raii::DescriptorSetLayout(device, createInfo);
}

vk::WriteDescriptorSet DescriptorBindings::getWriteSet(uint32_t                       binding,
                                                       const vk::raii::DescriptorSet& dstSet,
                                                       uint32_t                       dstArrayElement /*= ~0U*/,
                                                       uint32_t                       descriptorCount /*= 1*/) const
{
    vk::WriteDescriptorSet writeSet{};

    if (binding >= m_bindingToIndex.size()) {
        LOGE("`binding` was out of range");
        return writeSet;
    }

    const uint32_t i = m_bindingToIndex[binding];
    if (i == NO_BINDING_INDEX) {
        LOGE("`binding` has value of NO_BINDING_INDEX");
        return writeSet;
    }

    const auto& b = m_bindings[i];

    writeSet.setDstSet(dstSet);
    writeSet.setDstBinding(binding);
    writeSet.setDstArrayElement(dstArrayElement == ~0U ? 0 : dstArrayElement);
    writeSet.setDescriptorCount(dstArrayElement == ~0U ? b.descriptorCount : descriptorCount);
    writeSet.setDescriptorType(b.descriptorType);

    // Make sure that we don't exceed the array index when explicitly creating the write set if the binding is an array of descriptor
    assert(writeSet.dstArrayElement + writeSet.descriptorCount <= b.descriptorCount);

    return writeSet;
}

std::vector<vk::DescriptorPoolSize> DescriptorBindings::calculatePoolSizes(uint32_t numSets /*= 1*/) const {
    std::unordered_map<vk::DescriptorType, uint32_t> counts;
    for (auto& binding : m_bindings) {
        // Bindings can have zero descriptor count, in this case, don't reserve storage for them
        if (binding.descriptorCount == 0) {
            continue;
        }

        counts[binding.descriptorType] += binding.descriptorCount * numSets;
    }
    
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (const auto& [type, count] : counts) {
        poolSizes.push_back({
            .type            = type,
            .descriptorCount = count 
        });
    }
    return poolSizes;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DescriptorSetContainer::init(const DescriptorBindings&          bindings, 
                                  const vk::raii::Device&            device,
                                  uint32_t                           numSets     /*= 1 */,
                                  vk::DescriptorSetLayoutCreateFlags layoutFlags /*= {}*/,
                                  vk::DescriptorPoolCreateFlags      poolFlags   /*= {}*/)
{
    assert(m_device == nullptr && "initFromBindings must not be called twice in a row");
    m_device = &device;

    m_bindings = bindings;
    m_layout   = m_bindings.createDescriptorSetLayout(device, layoutFlags);

    if (numSets > 0) {
        // Descriptor Pool
        const std::vector<vk::DescriptorPoolSize> poolSizes{m_bindings.calculatePoolSizes(numSets)};
        const vk::DescriptorPoolCreateInfo poolCreateInfo{
            .flags         = poolFlags,
            .maxSets       = numSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes    = poolSizes.data()
        };
        m_pool = vk::raii::DescriptorPool(*m_device, poolCreateInfo);

        // Descriptor Sets
        m_sets.reserve(numSets);
        std::vector<vk::DescriptorSetLayout> allocInfoLayouts(numSets, m_layout);
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool     = m_pool,
            .descriptorSetCount = numSets,
            .pSetLayouts        = allocInfoLayouts.data()
        };
        m_sets = vk::raii::DescriptorSets(*m_device, allocInfo);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void WriteSetContainer::append(const vk::WriteDescriptorSet& writeSet, const poki::Buffer& buffer, vk::DeviceSize offset /*= 0*/, vk::DeviceSize range /*= vk::WholeSize*/) {
    assert(writeSet.pImageInfo == nullptr);
    assert(writeSet.pTexelBufferView == nullptr);
    assert(writeSet.descriptorCount == 1);
    
    vk::DescriptorBufferInfo bufferInfo{
        .buffer = buffer.buffer,
        .offset = offset,
        .range  = range
    };

    // emplace_back return a reference to the newly emplaced element
    // Point to a dummy non-null pointer so that we know this will need to be updated
    m_writeSets.emplace_back(writeSet).setPBufferInfo((const vk::DescriptorBufferInfo*)1);

    m_bufferOrImageDatas.emplace_back(bufferInfo);

    m_needPointerUpdate = true;
}

void WriteSetContainer::append(const vk::WriteDescriptorSet& writeSet, const poki::Image& image, std::optional<vk::ImageLayout> overrideLayout) {
    assert(writeSet.pBufferInfo == nullptr);
    assert(writeSet.pTexelBufferView == nullptr);
    assert(writeSet.descriptorCount == 1);
    assert(image.imageView != nullptr);
        
    vk::DescriptorImageInfo imageInfo{
        .sampler     = image.sampler,
        .imageView   = image.imageView,
        .imageLayout = overrideLayout.has_value() ? overrideLayout.value() : image.layout
    };

    m_writeSets.emplace_back(writeSet).setPImageInfo((const vk::DescriptorImageInfo*)1);

    m_bufferOrImageDatas.emplace_back(imageInfo);

    m_needPointerUpdate = true;
}


// TODO: Update to handle pImageInfo
const std::vector<vk::WriteDescriptorSet>& WriteSetContainer::data() {
    if (m_needPointerUpdate) {
        size_t bufferOrImageIndex{0};

        for (size_t i{0}; i < m_writeSets.size(); ++i) {
            if (m_writeSets[i].pBufferInfo) {
                m_writeSets[i].pBufferInfo = &m_bufferOrImageDatas[bufferOrImageIndex].buffer;
                bufferOrImageIndex += m_writeSets[i].descriptorCount;
            }
            if (m_writeSets[i].pImageInfo) {
                m_writeSets[i].pImageInfo = &m_bufferOrImageDatas[bufferOrImageIndex].image;
                bufferOrImageIndex += m_writeSets[i].descriptorCount;
            }
        }
    }

    return m_writeSets;
}


}; // namespace poki

