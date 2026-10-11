// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Asset/Asset.hpp"
#include "Surge/ScriptEngine/ScriptField.hpp"
#include <unordered_map>

namespace Surge
{
    class Entity;
    class Scene;
    class Script;
    struct ScriptInstance;

    // One Lua state for the whole application
    // - Scripts on entities run in their own environment (falling back to _G) and are driven through ScriptInstance
    // - require("Scripts.Game.State") loads another script asset as a module: run once per session in its own environment, then shared,
    //   so state can be shared between scripts without _G. Works on every platform (cooked bytecode, no filesystem lookups)
    class ScriptEngine
    {
    public:
        void Initialize();
        void Shutdown();

        // Compile
        // Takes raw Lua source code and returns compiled binary bytecode
        // @param source:    The raw Lua source code as a string
        // @param chunkName: Shown in errors and tracebacks, use "@" + the asset path (e.g. "@Scripts/Player.lua")
        // @return           A vector of bytes containing the compiled Lua bytecode, empty on a syntax error
        Vector<Byte> Compile(const String& source, const String& chunkName);

        // Runs the script in a fresh environment, writes the Inspector values into its Properties table and caches its callbacks.
        // Every callback receives the owner entity. Returns false if the script failed to load (the instance stays invalid)
        bool Instantiate(const Script& script, ScriptInstance& instance, Entity owner, const Vector<ScriptFieldOverride>& fields);
        void CallOnCreate(ScriptInstance& instance);
        void CallOnEnable(ScriptInstance& instance);
        void CallOnDisable(ScriptInstance& instance);
        void CallOnUpdate(ScriptInstance& instance, float deltaTime);
        void CallOnLateUpdate(ScriptInstance& instance, float deltaTime);
        void CallOnFixedUpdate(ScriptInstance& instance, float fixedDeltaTime);
        void CallOnDestroy(ScriptInstance& instance);
        void CallOnCollisionEnter(ScriptInstance& instance, Entity other);
        void CallOnCollisionExit(ScriptInstance& instance, Entity other);
        void CallOnTriggerEnter(ScriptInstance& instance, Entity other);
        void CallOnTriggerExit(ScriptInstance& instance, Entity other);

        // Script time (Lua: Time.*). The running scene advances it once per frame with the scaled and the real delta time
        void AdvanceTime(float deltaTime, float unscaledDeltaTime);
        float GetDeltaTime() const { return mDeltaTime; }
        float GetUnscaledDeltaTime() const { return mUnscaledDeltaTime; }
        double GetTime() const { return mTime; }
        double GetUnscaledTime() const { return mUnscaledTime; }
        uint64_t GetFrameCount() const { return mFrameCount; }

        // File name without extension of a script asset ("Scripts/Player.lua" -> "Player"), what GetScript("Player") matches
        static String GetScriptName(const Ref<Asset>& scriptAsset);

        // Writes values into the instance's Properties table (Instantiate does it before OnCreate, the Editor for live edits in Play mode).
        // A value whose type no longer matches the script's default is skipped with a warning
        void SetFields(ScriptInstance& instance, const Vector<ScriptFieldOverride>& fields, Scene* scene);

        // ResetSession
        // Fresh Lua world for a new play session: unloads every module and removes globals scripts added to _G, then collects garbage.
        void ResetSession();

        // Returns sol::state_view*, use with caution and cast to sol::state_view* when needed. This is to avoid exposing sol directly in the header
        void* GetSOLState() const;

    private:
        friend struct ModuleLoaderAccess;
        AssetID ResolveModule(const String& name, String& outPath, String& outError); // Module name -> registered script asset
        void RebuildModuleIndex();

    private:
        std::unordered_map<String /*Lowercase asset path ("scripts/game/state.lua")*/, AssetID /*Script AssetID*/> mModuleIndex;

        float mDeltaTime = 0.0f;
        float mUnscaledDeltaTime = 0.0f;
        double mTime = 0.0;          // Scaled seconds since the play session started
        double mUnscaledTime = 0.0;
        uint64_t mFrameCount = 0;
    };
}
