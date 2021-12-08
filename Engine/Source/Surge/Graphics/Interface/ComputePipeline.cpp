// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/ComputePipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanComputePipeline.hpp"

namespace Surge
{
    Ref<ComputePipeline> ComputePipeline::Create(Ref<Shader>& computeShader)
    {
        return Ref<VulkanComputePipeline>::Create(computeShader);
    }

} // namespace Surge
