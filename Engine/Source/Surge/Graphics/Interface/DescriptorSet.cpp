// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/DescriptorSet.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDescriptorSet.hpp"

namespace Surge
{
    Ref<DescriptorSet> DescriptorSet::Create(const Ref<Shader>& shader, Uint setNumber, bool resetEveryFrame, int index)
    {
        return Ref<VulkanDescriptorSet>::Create(shader, setNumber, resetEveryFrame, index);
    }

} // namespace Surge
