// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/Vector.hpp"
#include "SurgeReflect/SurgeReflect.hpp"
#include "Surge/ScriptEngine/Lua.hpp"
#include "Surge/ScriptEngine/ScriptField.hpp"

namespace Surge
{
    // Runtime state of a script on an entity, created by ScriptEngine::Instantiate() when the scene runs
    struct ScriptInstance
    {
        sol::environment Env; // The script's globals (falls back to _G), entity:GetScript() returns it
        sol::object Self;     // The owning Entity as Lua userdata, made once and passed to every callback (no allocation per call)

        // Callbacks the script defined, invalid (skipped) otherwise
        sol::protected_function OnCreate;
        sol::protected_function OnEnable;
        sol::protected_function OnDisable;
        sol::protected_function OnUpdate;
        sol::protected_function OnLateUpdate;
        sol::protected_function OnFixedUpdate;
        sol::protected_function OnDestroy;
        sol::protected_function OnCollisionEnter;
        sol::protected_function OnCollisionExit;
        sol::protected_function OnTriggerEnter;
        sol::protected_function OnTriggerExit;

        bool IsValid() const { return Env.valid(); }
    };

    // One script attached to an entity
    struct ScriptEntry
    {
        bool Active = true;
        Ref<Asset> ScriptAsset = nullptr;
        Vector<ScriptFieldOverride> Fields; // Inspector values of the script's Properties, saved with the scene

        // Runtime
        ScriptInstance Instance;
        bool IsInstantiated = false; // OnCreate ran (or the script failed to load, reported once)
        bool IsEnabled = false;      // OnEnable ran and OnDisable has not
        bool StartedThisFrame = false; // OnCreate ran this frame instead of OnUpdate: skips that frame's OnLateUpdate
    };

    // Any number of scripts per entity, run in list order. Each has its own environment and Properties
    struct ScriptComponent
    {
        bool Active = true;          // Turns every script of the entity off (OnDisable) and on (OnEnable)
        Vector<ScriptEntry> Scripts;

        // Scripts added from Lua (ScriptComponent:AddScript) join at the start of the next frame, never while the list is iterated
        Vector<Ref<Asset>> PendingScripts;

        SURGE_REFLECTION_ENABLE;
    };

    // Screen space UI canvas
    // Layout     : UILayout asset (.sui, made in the UI Editor), instantiated when the scene starts running
    // ScriptAsset: optional Lua script driving the canvas, find layout widgets with entity.UICanvasC:FindWidget("Name")
    //              or build widgets in code and call SetUIRoot(root) (attaches to this canvas)
    // SortOrder  : canvases with a higher sort order draw on top and receive input first
    struct UICanvasComponent
    {
        bool Active = true;
        bool ShowCanvas = true;
        Ref<Asset> Layout = nullptr;
        int SortOrder = 0;
        Ref<Asset> ScriptAsset = nullptr;
        bool IsInstantiated = false;
        uint64_t RuntimeCanvasID = 0;       // Entity UUID, key of this canvas in UI::Manager while the scene runs (0 = not registered)
        Vector<ScriptFieldOverride> Fields; // Inspector values of the script's Properties, saved with the scene

        ScriptInstance Instance;

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge
