// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/ShaderSet.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <json/json.hpp>
#include <filesystem>

namespace Surge
{
    void ShaderSet::Initialize(const String& baseShaderPath)
    {
        mBaseShaderPath = baseShaderPath;
        std::filesystem::create_directory(TEMP_ASSET_PATH);
        std::filesystem::create_directory(SHADER_CACHE_PATH);
    }

    void ShaderSet::AddShader(const String& shaderName)
    {
        mShaders.push_back(Shader::Create(fmt::format("{0}/{1}", mBaseShaderPath, shaderName)));
    }

    Ref<Shader>& ShaderSet::GetShader(const String& shaderName)
    {
        for (Ref<Shader>& shader : mShaders)
        {
            String nameWithoutExtension = Filesystem::RemoveExtension(shader->GetPath());
            if (nameWithoutExtension == fmt::format("{0}/{1}", mBaseShaderPath, shaderName))
                return shader;
        }

        SG_ASSERT_INTERNAL("No shaders found with name: {0}", shaderName);
        return mDummyShader;
    }

    void ShaderSet::LoadAll()
    {
        for (Ref<Shader>& shader : mShaders)
        {
            bool saveHash = false;

            const HashMap<ShaderType, String>& sources = shader->GetSources();
            HashMap<ShaderType, bool> forceCompileStages;
            for (auto& source : sources)
            {
                const HashCode& currentHashCode = shader->GetHash(source.first);
                const HashCode cacheHashCode = GetHashCodeFromCache(shader, source.first);

                String cachePath = GetCachePath(shader->GetPath(), source.first);
                bool existsInCache = Filesystem::Exists(cachePath);

                if (!existsInCache || (existsInCache && cacheHashCode != currentHashCode))
                {
                    forceCompileStages[source.first] = true;
                    saveHash = true;
                }
                else
                    forceCompileStages[source.first] = false;
            }

            shader->Load(forceCompileStages);
            CacheRequiredSPIRVs(shader, forceCompileStages);

            if (saveHash)
                WriteHashToFile(shader);
        }
    }

    void ShaderSet::Shutdown()
    {
        mShaders.clear();
    }

    HashCode ShaderSet::GetHashCodeFromCache(const Ref<Shader>& shader, ShaderType type)
    {
        FILE* f = nullptr;
        errno_t e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "r");
        String previousContents;
        if (f)
        {
            //Get the size
            fseek(f, 0, SEEK_END);
            uint64_t size = ftell(f);
            fseek(f, 0, SEEK_SET);

            // Read all the previous contents to a String
            previousContents.resize(size / sizeof(char));
            fread(previousContents.data(), sizeof(char), previousContents.size(), f);
            fclose(f);
        }

        String name = GetCacheName(shader->GetPath(), type);
        nlohmann::json j = previousContents.empty() ? nlohmann::json() : nlohmann::json::parse(previousContents);

        HashCode result = 0;
        if (j.contains(name))
            result = j[name];
        return result;
    }

    void ShaderSet::CacheRequiredSPIRVs(const Ref<Shader>& shader, const HashMap<ShaderType, bool>& stagesToCache)
    {
        for (auto& stage : stagesToCache)
        {
            if (!stage.second)
                continue;

            String path = GetCachePath(shader->GetPath(), stage.first);
            FILE* f;
            fopen_s(&f, path.c_str(), "wb");
            if (f)
            {
                SPIRVHandle spirvToCache;
                Vector<SPIRVHandle> spirvs = shader->GetSPIRVs();
                for (SPIRVHandle& spirvHandle : spirvs)
                {
                    if (spirvHandle.Type == stage.first)
                    {
                        spirvToCache = spirvHandle;
                        break;
                    }
                }

                fwrite(spirvToCache.SPIRV.data(), sizeof(Uint), spirvToCache.SPIRV.size(), f);
                fclose(f);
                Log<Severity::Debug>("Cached Shader at: {0}", path);
            }
        }
    }

    void ShaderSet::WriteHashToFile(const Ref<Shader>& shader)
    {
        FILE* f = nullptr;
        errno_t e;

        // Create the file if it doesn't exist
        if (!Filesystem::Exists(SHADER_HASH_CACHE_PATH))
        {
            fopen_s(&f, SHADER_HASH_CACHE_PATH, "w");
            if (f)
                fclose(f);
        }


        e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "r");

        // If the file doesn't exists, create an empty file
        if (e == ENOENT)
        {
            e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "w");
            if (f)
                fclose(f);
        }

        // Load in the contents of the file, because we append to it later
        String previousContents;
        if (f)
        {
            fseek(f, 0, SEEK_END);
            uint64_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            previousContents.resize(size / sizeof(char));
            fread(previousContents.data(), sizeof(char), previousContents.size(), f);
            fclose(f);
        }

        // Create A JSON, from the previous file for writing the new contents
        nlohmann::json j = previousContents.empty() ? nlohmann::json() : nlohmann::json::parse(previousContents);

        // For each HashCodes, update it
        for (auto& e : shader->GetHashCodes())
        {
            String name = GetCacheName(shader->GetPath(), e.first);
            j[name] = e.second;
        }

        String result = j.dump(4);
        e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

    String ShaderSet::GetCachePath(const Path& shaderPath, const ShaderType& type) const
    {
        String path = fmt::format("{0}/{1}", SHADER_CACHE_PATH, GetCacheName(shaderPath, type));
        return path;
    }

    String ShaderSet::GetCacheName(const Path& shaderPath, const ShaderType& type) const
    {
        String name = fmt::format("{0}.{1}.spv", Filesystem::GetNameWithExtension(shaderPath), ShaderTypeToString(type));
        return name;
    }
}
