// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"

namespace Surge
{
    // Compiles sourcePath into cookedPath when the cooked file is missing or outdated (Editor only, see ShaderCompiler)
    using ShaderCompileHook = void (*)(const String& sourcePath, const String& cookedPath, ShaderType types);

    class ShaderManager
    {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;
        SURGE_DISABLE_COPY(ShaderManager);

        void Initialize(const String& baseShaderPath);

        // Shader name in baseShaderPath directory, with extension, e.g. "Renderer3D.glsl"
        // types must match the //SURGE:[Shader: X] stages present in the file, e.g. ShaderType::COMPUTE for compute shaders
        void Load(const String& shaderName, ShaderType types = ShaderType::VERTEX | ShaderType::FRAGMENT);
        void Shutdown();

        const Shader& Get(const String& shaderName) const; //Name with extension
        const Vector<Shader>& GetAllShaders() const { return mShaders; }

        // Set before Core::Initialize, the Renderer loads the engine shaders while initializing
        static void SetCompileHook(ShaderCompileHook hook) { sCompileHook = hook; }
        static ShaderCompileHook GetCompileHook() { return sCompileHook; }

    private:
        String mBaseShaderPath;
        Vector<Shader> mShaders;
        Shader mDummyShader {};
        static inline ShaderCompileHook sCompileHook = nullptr;
    };

} // namespace Surge
