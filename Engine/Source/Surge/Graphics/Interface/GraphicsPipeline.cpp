// Copyright (c) - SurgeTechnologies - All rights reserved
#include "GraphicsPipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"

namespace Surge
{
    Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineSpecification& pipelineSpec)
    {
        return Ref<VulkanGraphicsPipeline>::Create(pipelineSpec);
    }

} // namespace Surge
