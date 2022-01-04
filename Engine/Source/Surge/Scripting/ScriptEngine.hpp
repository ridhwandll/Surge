// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Scripting/Compiler/ScriptCompiler.hpp"

namespace Surge
{
    using ScriptID = UUID;

    class SurgeBehaviour;
    class Scene;
    struct SURGE_API ScriptInstance
    {
        Path ScriptPath; // Relative to project
        SurgeReflect::Class* Reflection;
        SurgeBehaviour* Script;
        void* LibHandle;
    };

    // Main class SURGE_API  for C++ scripting system in Surge, mainly managed by the Surge::Project class SURGE_API
    // Owns a Surge::ScriptCompiler and all the scripts per project(managed via a HashMap)
    class SURGE_API ScriptEngine
    {
    public:
        ScriptEngine() = default;
        ~ScriptEngine() = default;

        void Initialize();
        void Shutdown();
        void CompileScripts();

        // Script Manipulation
        ScriptID CreateScript(const Path& scriptPath);
        void DestroyScript(ScriptID& handle);
        const ScriptInstance& GetScript(const ScriptID& handle) const;
        const auto& GetAllScripts() const { return mScripts; }
        ScriptCompiler* GetCompiler() { return mCompiler; };
        bool IsScriptValid(ScriptID& handle);

    private:
        // Function called by Surge::Project
        void OnRuntimeStart();
        void OnUpdate();
        void OnRuntimeEnd();
        void OnSceneChange(Scene* activatedScene);

    private:
        ScriptCompiler* mCompiler;
        HashMap<ScriptID, ScriptInstance> mScripts; // Mapped as <ScriptID - ScriptInstance>
        friend class SURGE_API Project;
    };

} // namespace Surge
