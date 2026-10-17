// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Shader/ShaderCompiler.hpp"
#include "Shader/ShaderReflector.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Graphics/Shader/ShaderBinaryFormat.hpp"
#include "Surge/Graphics/RHI/Vulkan/VulkanUtils.hpp"
#include "Surge/Utility/Filesystem.hpp"

#include <shaderc/shaderc.hpp>
#include <algorithm>

namespace Surge::ShaderCompiler
{
    struct StageSource
    {
        ShaderType Type = ShaderType::NONE;
        String Source;
    };

    // Resolves #include "File.glsli" relative to the directory of the shader being compiled and records what it resolved
    // Shaders using it need: #extension GL_GOOGLE_include_directive : require
    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        ShaderIncluder(String baseDirectory, Vector<String>& outIncludes)
            : mBaseDirectory(std::move(baseDirectory)), mIncludes(outIncludes) {}

        shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type /*type*/, const char* /*requestingSource*/, size_t /*includeDepth*/) override
        {
            IncludeData* data = new IncludeData();
            data->SourceName = std::format("{0}/{1}", mBaseDirectory, requestedSource);
            if (Filesystem::ReadTextFile(data->SourceName, data->Content))
                mIncludes.push_back(requestedSource);
            else
            {
                // shaderc convention: empty source_name signals failure, content holds the error message
                data->Content = std::format("Cannot open include file '{0}'", data->SourceName);
                data->SourceName.clear();
            }

            data->Result.source_name = data->SourceName.c_str();
            data->Result.source_name_length = data->SourceName.size();
            data->Result.content = data->Content.c_str();
            data->Result.content_length = data->Content.size();
            data->Result.user_data = data;
            return &data->Result;
        }

        void ReleaseInclude(shaderc_include_result* data) override
        {
            delete static_cast<IncludeData*>(data->user_data);
        }

    private:
        struct IncludeData
        {
            shaderc_include_result Result = {};
            String SourceName;
            String Content;
        };
        String mBaseDirectory;
        Vector<String>& mIncludes;
    };

    static shaderc_shader_kind ShadercShaderKindFromSurgeShaderType(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::VERTEX: return shaderc_glsl_vertex_shader;
        case ShaderType::FRAGMENT: return shaderc_glsl_fragment_shader;
        case ShaderType::COMPUTE: return shaderc_glsl_compute_shader;
        case ShaderType::NONE: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }

        return static_cast<shaderc_shader_kind>(-1);
    }

    // Splits the file at its //SURGE:[Shader: X] markers
    static bool ParseStages(const String& sourcePath, const String& source, Vector<StageSource>& outStages, ShaderType& outTypes)
    {
        const char* typeToken = "//SURGE:[Shader:";
        const size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != String::npos)
        {
            const size_t eol = source.find_first_of("\r\n", pos);
            const size_t begin = pos + typeTokenLength + 1;
            String type = source.substr(begin, eol - begin);
            type.pop_back(); // Remove the closing ']' character

            const ShaderType shaderType = VulkanUtils::ShaderTypeFromString(type);
            if (shaderType == ShaderType::NONE)
            {
                Log<Severity::Error>("[ShaderCompiler] {}: unknown stage marker '{}'", sourcePath, type);
                return false;
            }

            const size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find(typeToken, nextLinePos);
            outStages.push_back({ shaderType, (pos == String::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos) });
            outTypes |= shaderType;
        }
        return true;
    }

    static bool CompileStage(const shaderc::Compiler& compiler, const String& sourcePath, const StageSource& stage, bool optimize, Vector<String>& outIncludes, Vector<Uint>& outSPIRV)
    {
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
        options.SetIncluder(std::make_unique<ShaderIncluder>(Filesystem::GetParentPath(sourcePath).string(), outIncludes));
        if (optimize)
            options.SetOptimizationLevel(shaderc_optimization_level_performance);

        const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(stage.Source, ShadercShaderKindFromSurgeShaderType(stage.Type), sourcePath.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            Log<Severity::Error>("[ShaderCompiler] {} ({}) failed to compile, {} error(s):\n{}", sourcePath, VulkanUtils::ShaderTypeToString(stage.Type), result.GetNumErrors(), result.GetErrorMessage());
            return false;
        }

        outSPIRV.assign(result.cbegin(), result.cend());
        return !outSPIRV.empty();
    }

    void CompileIfOutdated(const String& sourcePath, const String& cookedPath, ShaderType types)
    {
        if (!IsUpToDate(sourcePath, cookedPath))
            Compile(sourcePath, cookedPath, types);
    }

    bool IsUpToDate(const String& sourcePath, const String& cookedPath)
    {
        uint64_t hash = 0;
        Vector<String> includes;
        if (!ShaderBinary::ReadSourceInfo(cookedPath, hash, includes))
            return false;
        return ShaderBinary::HashSource(sourcePath, includes) == hash;
    }

    bool Compile(const String& sourcePath, const String& cookedPath, ShaderType types)
    {
        String source;
        if (!Filesystem::ReadTextFile(sourcePath, source))
        {
            Log<Severity::Error>("[ShaderCompiler] Failed to read {}", sourcePath);
            return false;
        }

        Vector<StageSource> stages;
        ShaderType foundTypes = ShaderType::NONE;
        if (!ParseStages(sourcePath, source, stages, foundTypes))
            return false;
        if (foundTypes != types)
        {
            Log<Severity::Error>("[ShaderCompiler] {} contains the stages {} but is loaded as {}, pass the matching ShaderType to ShaderManager::Load", sourcePath, VulkanUtils::ShaderTypeToString(foundTypes), VulkanUtils::ShaderTypeToString(types));
            return false;
        }

        shaderc::Compiler compiler;
        ShaderBinary::CookedShader cooked;
        cooked.Stages = types;

        // The optimizer strips names, so reflection runs on unoptimized SPIR-V. Bindings/locations are decorations and survive it
        // Resources the optimizer drops stay in the layouts, a superset of what the shipped SPIR-V uses is valid
        Vector<SPIRVHandle> reflectionSPIRVs;
        for (const StageSource& stage : stages)
        {
            SPIRVHandle unoptimized = { {}, stage.Type };
            SPIRVHandle optimized = { {}, stage.Type };
            if (!CompileStage(compiler, sourcePath, stage, false, cooked.Includes, unoptimized.SPIRV) || !CompileStage(compiler, sourcePath, stage, true, cooked.Includes, optimized.SPIRV))
                return false;

            reflectionSPIRVs.push_back(std::move(unoptimized));
            cooked.SPIRVs.push_back(std::move(optimized));
        }

        std::sort(cooked.Includes.begin(), cooked.Includes.end());
        cooked.Includes.erase(std::unique(cooked.Includes.begin(), cooked.Includes.end()), cooked.Includes.end());
        cooked.SourceHash = ShaderBinary::HashSource(sourcePath, cooked.Includes);
        if (cooked.SourceHash == 0)
        {
            Log<Severity::Error>("[ShaderCompiler] Failed to hash {} and its includes", sourcePath);
            return false;
        }

        ShaderReflector reflector;
        cooked.Reflection = reflector.Reflect(Filesystem::GetFilenameWithExt(sourcePath), reflectionSPIRVs);

        if (!ShaderBinary::Write(cookedPath, cooked))
            return false;

        Log<Severity::Info>("[ShaderCompiler] Compiled {}", sourcePath);
        return true;
    }

    bool CookLoadedShaders(Vector<String>& outFailed)
    {
        for (const Shader& shader : Core::GetRenderer()->GetShaderManager().GetAllShaders())
        {
            const String cookedPath = ShaderBinary::GetCookedPath(shader.GetPath());
            if (!IsUpToDate(shader.GetPath(), cookedPath) && !Compile(shader.GetPath(), cookedPath, shader.GetTypes()))
                outFailed.push_back(shader.GetPath());
        }
        return outFailed.empty();
    }
}
