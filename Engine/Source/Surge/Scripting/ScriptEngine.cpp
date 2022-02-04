// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Scripting/ScriptEngine.hpp"
#include "Surge/Scripting/Compiler/CompilerMSVC.hpp"
#include "SurgeReflect/SurgeReflectRegistry.hpp"
#include "Surge/Scripting/SurgeBehaviour.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/ECS/Scene.hpp"

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

    ScriptID ScriptEngine::CreateScript(const Path& scriptPath, const UUID& entityID)
    {
        ScriptID id = UUID();
        ScriptInstance newScriptInstance = {};
        newScriptInstance.ScriptSourcePath = scriptPath;
        newScriptInstance.ParentEntityID = entityID;
        newScriptInstance.Reflection = nullptr; // Filled later
        newScriptInstance.Script = nullptr;     // Filled later
        mScripts[id] = newScriptInstance;
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

            scriptInstance.Script = scriptCreateFN(scene->FindEntityByUUID(scriptInstance.ParentEntityID));
            scriptInstance.Reflection = getReflectionFN();

            SG_ASSERT_NOMSG(scriptInstance.LibHandle);
            SG_ASSERT_NOMSG(scriptInstance.Reflection);
            SG_ASSERT_NOMSG(scriptInstance.Script);

            if (scriptInstance.Reflection->GetFunction("OnStart"))
            {
                scriptInstance.Script->OnStart();
            }
        }
    }

    void ScriptEngine::OnUpdate()
    {
        for (auto [scriptID, scriptInstance] : mScripts)
        {
            if (scriptInstance.Reflection->GetFunction("OnUpdate"))
            {
                scriptInstance.Script->OnUpdate();
            }
        }
    }

    void ScriptEngine::OnRuntimeEnd()
    {
        SurgeReflect::Registry* reflection = SurgeReflect::Registry::Get();
        for (auto [scriptID, scriptInstance] : mScripts)
        {
            SG_ASSERT_NOMSG(scriptInstance.LibHandle);
            SG_ASSERT_NOMSG(scriptInstance.Reflection);
            SG_ASSERT_NOMSG(scriptInstance.Script);

            const String& className = scriptInstance.Reflection->GetName();
            if (scriptInstance.Reflection->GetFunction("OnDestroy"))
            {
                scriptInstance.Script->OnDestroy();
            }
            reflection->RemoveClass(className);
            DestroyScriptFN scriptDestroyFN = reinterpret_cast<DestroyScriptFN>(Platform::GetFunction(scriptInstance.LibHandle, "DestroyScript"));
            scriptDestroyFN(scriptInstance.Script);
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
