// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include <fstream>
#include <shaderc/shaderc.hpp>

namespace Surge
{
    VulkanShader::VulkanShader(const Path& path)
        : mPath(path)
    {
        Reload();
    }

    VulkanShader::~VulkanShader()
    {

    }

    void VulkanShader::Reload()
    {
        //TODO: Implement
        //ParseShader();
        Compile();
    }

    void VulkanShader::Compile()
    {
        //TODO: Implement
        shaderc::Compiler compiler;
        shaderc::CompilationResult res = compiler.CompileGlslToSpv("cake", shaderc_glsl_fragment_shader, "KEKW");
        auto kek = res.GetCompilationStatus();
        switch (kek)
        {
        case shaderc_compilation_status_success:               Log("shaderc_compilation_status_success"); break;
        case shaderc_compilation_status_invalid_stage:         Log("shaderc_compilation_status_invalid_stage"); break;
        case shaderc_compilation_status_compilation_error:     Log("shaderc_compilation_status_compilation_error"); break;
        case shaderc_compilation_status_internal_error:        Log("shaderc_compilation_status_internal_error"); break;
        case shaderc_compilation_status_null_result_object:    Log("shaderc_compilation_status_null_result_object"); break;
        case shaderc_compilation_status_invalid_assembly:      Log("shaderc_compilation_status_invalid_assembly"); break;
        case shaderc_compilation_status_validation_error:      Log("shaderc_compilation_status_validation_error"); break;
        case shaderc_compilation_status_transformation_error:  Log("shaderc_compilation_status_transformation_error"); break;
        case shaderc_compilation_status_configuration_error:   Log("shaderc_compilation_status_configuration_error"); break;
        }
    }

    void VulkanShader::ParseShader()
    {
        std::ifstream stream = std::ifstream(mPath);
        if (stream)
        {
            
        }
        else
            SG_ASSERT_INTERNAL("Cannot open path \"{0}\"", mPath);
    }
}
