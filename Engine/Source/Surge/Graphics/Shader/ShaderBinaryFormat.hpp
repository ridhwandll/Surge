// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge::ShaderBinary
{
    // Engine shaders, compiled by the Editor's ShaderCompiler: SPIR-V of every stage + its reflection, loaded as is on every platform
    // Layout: Header, include list, stages (type + SPIR-V words), ShaderReflectionData
    constexpr Uint MAGIC = 0x52444853; // SHDR
    constexpr Uint VERSION = 1;

    struct Header
    {
        Uint Magic = MAGIC;
        Uint Version = VERSION;
        uint64_t SourceHash = 0;
        Uint Stages = 0;
        Uint IncludeCount = 0;
    };
    static_assert(sizeof(Header) == 24);

    struct CookedShader
    {
        uint64_t SourceHash = 0;
        ShaderType Stages = ShaderType::NONE;
        Vector<String> Includes;  // As written in #include, relative to the shader's directory
        Vector<SPIRVHandle> SPIRVs;
        ShaderReflectionData Reflection;
    };

    // "Engine/Assets/Shaders/Renderer3D.glsl" -> "Engine/Assets/Shaders/Cooked/Renderer3D.rSHADER"
    String GetCookedPath(const String& sourcePath);

    // Main file + every include (line endings ignored), 0 if one of them can't be read. Needs the sources, so never on Android
    uint64_t HashSource(const String& sourcePath, const Vector<String>& includes);

    bool Write(const String& path, const CookedShader& shader);
    bool Read(const String& path, CookedShader& outShader);

    bool ReadSourceInfo(const String& path, uint64_t& outHash, Vector<String>& outIncludes);
}
