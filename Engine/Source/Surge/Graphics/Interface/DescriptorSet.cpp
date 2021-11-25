// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/DescriptorSet.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDescriptorSet.hpp"

namespace Surge
{
    Ref<DescriptorSet> DescriptorSet::Create(const Ref<Shader>& shader, bool resetEveryFrame, int index)
    {
        return Ref<VulkanDescriptorSet>::Create(shader, resetEveryFrame, index);
    }

} // namespace Surge
