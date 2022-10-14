// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Scripting/ScriptEngine.hpp"
#include "Surge/Scripting/Compiler/CompilerMSVC.hpp"
#include "SurgeReflect/SurgeReflectRegistry.hpp"
#include "Surge/Scripting/SurgeBehaviour.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Surge/Core/Hash.hpp"

#define SCRIPT_BINARY_FOLDER_NAME "ScriptBinaries"
namespace Surge
{
    using CreateScriptFN = SurgeBehaviour* (*)(Entity& e);
    using GetReflectionFN = SurgeReflect::Class* (*)();
    using DestroyScriptFN = void (*)(SurgeBehaviour*);

    void ScriptEngine::Initialize()
    {
        mCompiler = new CompilerMSVC();
        mCompiler->Initialize();
        Log<Severity::Info>("ScriptEngine initialized with compiler: {0}", mCompiler->GetName());
    }

    ScriptID ScriptEngine::CreateScript(Path& scriptPath, const UUID& entityID)
    {
        Hash hasher;
        ScriptID id = hasher.Generate<String>(scriptPath);
        if (!HasDuplicate(id))
        {
            ScriptInstance newScriptInstance = {};
            newScriptInstance.ScriptSourcePath = scriptPath;
            newScriptInstance.ScriptAndParentEntityIDs.push_back({nullptr, entityID});
            newScriptInstance.Reflection = nullptr; // Filled later
            mScripts[id] = newScriptInstance;
        }
        else
        {
            mScripts[id].ScriptAndParentEntityIDs.push_back({nullptr, entityID});
        }
        return id;
    }

    const ScriptInstance& ScriptEngine::GetScript(const ScriptID& handle) const
    {
        return mScripts.at(handle);
    }

    bool ScriptEngine::IsScriptValid(ScriptID& handle)
    {
        auto itr = mScripts.find(handle);
        if (itr != mScripts.end())
        {
            return true;
        }
        return false;
    }

    void ScriptEngine::OnRuntimeStart(Scene* scene)
    {
        SurgeReflect::Registry* reg = SurgeReflect::Registry::Get();

        for (auto& [scriptID, scriptInstance] : mScripts)
        {
            Path scriptBinaryDir = GetScriptBinaryDir();
            // TODO: Don't harcode the .dll extension
            String libName = fmt::format("{0}/{1}.dll", scriptBinaryDir, scriptInstance.ScriptSourcePath.FileName());
            scriptInstance.LibHandle = Platform::LoadSharedLibrary(libName);

            if (scriptInstance.LibHandle == NULL)
            {
                CompileInfo opt;
                opt.InputFile = scriptInstance.ScriptSourcePath;
                mCompiler->CompileAndLink(scriptBinaryDir, opt);
                scriptInstance.LibHandle = Platform::LoadSharedLibrary(libName);
            }
            CreateScriptFN scriptCreateFN = reinterpret_cast<CreateScriptFN>(Platform::GetFunction(scriptInstance.LibHandle, "CreateScript"));
            GetReflectionFN getReflectionFN = reinterpret_cast<GetReflectionFN>(Platform::GetFunction(scriptInstance.LibHandle, "GetReflection"));

            for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
            {
                sc.Data1 = scriptCreateFN(scene->FindEntityByUUID(sc.Data2));
                scriptInstance.Reflection = getReflectionFN();
                SG_ASSERT_NOMSG(sc.Data1);
            }
            SG_ASSERT_NOMSG(scriptInstance.LibHandle);
            SG_ASSERT_NOMSG(scriptInstance.Reflection);
        }

        auto view = scene->GetRegistry().view<ScriptComponent>();
        for (auto& entity : view)
        {
            auto& script = view.get<ScriptComponent>(entity);
            auto& scriptInstance = mScripts[script.ScriptEngineID];
            if (scriptInstance.Reflection->GetFunction("OnStart"))
            {
                for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
                    sc.Data1->OnStart();
            }
        }        
    }

    void ScriptEngine::OnUpdate(Scene* scene)
    {
        auto view = scene->GetRegistry().view<ScriptComponent>();
        for (auto& entity : view)
        {
            auto& script = view.get<ScriptComponent>(entity);
            auto& scriptInstance = mScripts[script.ScriptEngineID];
            if (scriptInstance.Reflection->GetFunction("OnUpdate"))
            {
                for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
                {
                    sc.Data1->OnUpdate();
                }
            }
        }
    }

    void ScriptEngine::OnRuntimeEnd(Scene* scene)
    {
        // Call Destroy function of every script
        auto view = scene->GetRegistry().view<ScriptComponent>();
        for (auto& entity : view)
        {
            auto& script = view.get<ScriptComponent>(entity);
            auto& scriptInstance = mScripts[script.ScriptEngineID];
            if (scriptInstance.Reflection->GetFunction("OnDestroy"))
            {
                for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
                    sc.Data1->OnDestroy();
            }
        }        

        SurgeReflect::Registry* reflection = SurgeReflect::Registry::Get();
        for (auto [scriptID, scriptInstance] : mScripts)
        {
            for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
            {
                SG_ASSERT_NOMSG(sc.Data1);
            }

            SG_ASSERT_NOMSG(scriptInstance.LibHandle);
            SG_ASSERT_NOMSG(scriptInstance.Reflection);

            const String& className = scriptInstance.Reflection->GetName();
            reflection->RemoveClass(className);
            DestroyScriptFN scriptDestroyFN = reinterpret_cast<DestroyScriptFN>(Platform::GetFunction(scriptInstance.LibHandle, "DestroyScript"));
            for (auto& sc : scriptInstance.ScriptAndParentEntityIDs)
                scriptDestroyFN(sc.Data1);

            Platform::UnloadSharedLibrary(scriptInstance.LibHandle);
            scriptInstance.LibHandle = nullptr;
        }
    }

    void ScriptEngine::OnSceneChange(Scene* activatedScene)
    {
        mScripts.clear();
        auto& reg = activatedScene->GetRegistry();
        {
            const auto& view = reg.view<IDComponent, ScriptComponent>();
            for (auto& entity : view)
            {
                const auto& [id, script] = view.get<IDComponent, ScriptComponent>(entity);
                script.ScriptEngineID = CreateScript(script.ScriptPath, id.ID);
            }
        }
    }

    Surge::Path ScriptEngine::GetScriptBinaryDir()
    {
        Path res = Core::GetClient()->GetActiveProject().GetMetadata().InternalDirectory / SCRIPT_BINARY_FOLDER_NAME;
        Filesystem::CreateOrEnsureDirectory(res);
        return res;
    }

    bool ScriptEngine::HasDuplicate(ScriptID scriptID)
    {
        if (mScripts.find(scriptID) == mScripts.end())
            return false;

        return true;
    }

    void ScriptEngine::DestroyScript(ScriptID& handle)
    {
        if (!handle)
            return;

        mScripts.erase(handle);
        handle = NULL_UUID;
    }

    void ScriptEngine::Shutdown()
    {
        mScripts.clear();
        mCompiler->Shutdown();
    }

    void ScriptEngine::CompileScripts()
    {
        for (auto [scriptID, scriptInstance] : mScripts)
        {
            CompileInfo opt;
            opt.InputFile = scriptInstance.ScriptSourcePath;
            mCompiler->CompileAndLinkAsync(GetScriptBinaryDir(), opt);
        }
    }

} // namespace Surge
