// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanTexture.hpp"

namespace Surge
{
    Ref<Texture2D> Texture2D::Create(const String& filepath, TextureSpecification specification) { return Ref<VulkanTexture2D>::Create(filepath, specification); }

    Uint Texture::CalculateMipChainLevels(Uint width, Uint height) { return std::floor(std::log2(std::max(width, height))) + 1; }
} // namespace Surge
