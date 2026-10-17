// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ShaderBinaryFormat.hpp"
#include "Surge/Graphics/RHI/Vulkan/VulkanUtils.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge
{
    bool Shader::Load(const String& glslPath, ShaderType type)
    {
        mPath = glslPath;
        mTypesBit = type;
        mShaderSPIRVs.clear();
        mReflectionData = ShaderReflectionData();

        const String cookedPath = ShaderBinary::GetCookedPath(glslPath);
        ShaderBinary::CookedShader cooked;
        if (!ShaderBinary::Read(cookedPath, cooked))
        {
            Log<Severity::Fatal>("[Shader] No usable cooked shader for {}: open the Editor once to compile the engine shaders", glslPath);
            SG_ASSERT_INTERNAL("Missing cooked shader!");
            return false;
        }

        if (cooked.Stages != type)
        {
            Log<Severity::Fatal>("[Shader] {} was cooked with stages {} but is loaded as {}", glslPath, VulkanUtils::ShaderTypeToString(cooked.Stages), VulkanUtils::ShaderTypeToString(type));
            SG_ASSERT_INTERNAL("Shader stage mismatch!");
            return false;
        }

#if defined(SURGE_PLATFORM_WINDOWS) && defined(SURGE_DEBUG)
        // The Player never compiles, an edited shader only shows up once the Editor recompiled it
        if (Filesystem::Exists(glslPath) && ShaderBinary::HashSource(glslPath, cooked.Includes) != cooked.SourceHash)
            Log<Severity::Warn>("[Shader] {} changed since it was cooked, open the Editor to recompile it", glslPath);
#endif

        mShaderSPIRVs = std::move(cooked.SPIRVs);
        mReflectionData = std::move(cooked.Reflection);
        return true;
    }

} // namespace Surge
