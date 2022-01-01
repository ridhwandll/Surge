// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Scripting/ScriptEngine.hpp"
#include "Surge/Scripting/Compiler/CompilerMSVC.hpp"
#include "SurgeReflect/SurgeReflectRegistry.hpp"

namespace Surge
{
    using CreateScriptFN = SurgeBehaviour* (*)(SurgeReflect::Registry* reg);
    using GetReflectionFN = SurgeReflect::Class* (*)(SurgeReflect::Registry*);
    using DestroyScriptFN = void (*)(SurgeBehaviour*);

    void ScriptEngine::Initialize()
    {
        mCompiler = new CompilerMSVC();
        mCompiler->Initialize();
        Log<Severity::Info>("ScriptEngine initialized with compiler: {0}", mCompiler->GetName());
    }

    ScriptID ScriptEngine::CreateScript(const Path& scriptPath)
    {
        SG_ASSERT(mActiveProjctID, "No Active project!");

        ScriptID id = UUID();
        ScriptInstance newScriptInstance = {};
        newScriptInstance.ScriptPath = scriptPath;
        newScriptInstance.Reflection = nullptr; // Filled later
        newScriptInstance.Script = nullptr;     // Filled later
        mScripts[mActiveProjctID][id] = newScriptInstance;
        return id;
    }

    const ScriptInstance& ScriptEngine::GetScript(const ScriptID& handle) const
    {
        SG_ASSERT(mActiveProjctID, "No Active project!");
        return mScripts.at(mActiveProjctID).at(handle);
    }

    void ScriptEngine::OnRuntimeStart()
    {
        SurgeReflect::Registry* reg = SurgeReflect::Registry::Get();

        SG_ASSERT(mActiveProjctID, "No Active project!");
        HashMap<ScriptID, ScriptInstance>& k = mScripts[mActiveProjctID];
        for (auto& [scriptID, scriptInstance] : k)
        {
            // TODO: Don't harcode the .dll extension
            String libName = fmt::format("{0}/{1}.dll", scriptInstance.ScriptPath.ParentPath(), scriptInstance.ScriptPath.FileName());
            scriptInstance.LibHandle = Platform::LoadSharedLibrary(libName);

            CreateScriptFN scriptCreateFN = reinterpret_cast<CreateScriptFN>(Platform::GetFunction(scriptInstance.LibHandle, "CreateScript"));
            GetReflectionFN getReflectionFN = reinterpret_cast<GetReflectionFN>(Platform::GetFunction(scriptInstance.LibHandle, "GetReflection"));

            scriptInstance.Script = scriptCreateFN(reg);
            scriptInstance.Reflection = getReflectionFN(reg);

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
        for (auto [scriptID, scriptInstance] : mScripts[mActiveProjctID])
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
        for (auto [scriptID, scriptInstance] : mScripts[mActiveProjctID])
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

    void ScriptEngine::DestroyScript(const ScriptID& handle)
    {
        SG_ASSERT(mActiveProjctID, "No Active project!");
        mScripts.at(mActiveProjctID).erase(handle);
    }

    void ScriptEngine::Shutdown()
    {
        mScripts.clear();
        mCompiler->Shutdown();
    }

    void ScriptEngine::CompileScripts()
    {
        for (auto [scriptID, scriptInstance] : mScripts[mActiveProjctID])
        {
            CompileInfo opt;
            opt.InputFile = scriptInstance.ScriptPath;
            mCompiler->CompileAndLinkAsync(scriptInstance.ScriptPath.ParentPath(), opt);
        }
    }

} // namespace Surge
