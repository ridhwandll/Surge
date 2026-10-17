// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge::ShaderCompiler
{
    // GLSL (//SURGE:[Shader: X] stage markers, #include "File.glsli") -> ShaderBinary: optimized SPIR-V + reflection
    // Lives in the Editor only, so neither the Player nor exported builds link shaderc or SPIRV-Cross

    // ShaderManager compile hook, installed before Core::Initialize
    void CompileIfOutdated(const String& sourcePath, const String& cookedPath, ShaderType types);

    // False when the cooked file is missing, from an older ShaderBinary::VERSION, or the source/one of its includes changed
    bool IsUpToDate(const String& sourcePath, const String& cookedPath);

    // On failure the previous cooked file (if any) is left untouched
    bool Compile(const String& sourcePath, const String& cookedPath, ShaderType types);

    // Export: (re)compiles every shader the Renderer loaded that is outdated, false with the failed sources in outFailed
    bool CookLoadedShaders(Vector<String>& outFailed);
}
