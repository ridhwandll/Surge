// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/ScriptEngine/ScriptField.hpp"

namespace Surge
{
    class Scene;
    struct ScriptInstance;

    // A field a script declares in its Properties table, as the Inspector shows it
    struct ScriptFieldInfo
    {
        String Name;
        ScriptFieldType Type = ScriptFieldType::FLOAT;
        ScriptFieldValue Default;
        float Min = 0.0f; // Min < Max: the Inspector clamps to the range
        float Max = 0.0f;
        String Tooltip;

        AssetType AssetKind = AssetType::NONE;                // ASSET (or ARRAY of ASSET): accepted asset type, NONE = any
        Vector<String> Options;                               // ENUM
        ScriptFieldType ElementType = ScriptFieldType::FLOAT; // ARRAY
    };

    struct ScriptFieldSchema
    {
        Vector<ScriptFieldInfo> Fields; // Declaration order
        String Error;                   // The script failed to run, no fields are known
    };

    namespace ScriptFields
    {
        // Runs the script once in a throwaway environment where Property.* record types, ranges and tooltips.
        // Cached until the script is recooked
        const ScriptFieldSchema& GetSchema(const Ref<Asset>& scriptAsset);

        // Inspector rows, inside a 2 column property table. Edit mode: edits the saved overrides (values equal to the default are not stored).
        // Play mode with a running instance: reads and writes its Properties table live, discarded when Play stops like any runtime change.
        // Reloads the script when its source changed on disk. Returns true when a value changed
        bool DrawInspector(Ref<Asset>& scriptAsset, Vector<ScriptFieldOverride>& overrides, ScriptInstance* liveInstance, Scene* scene);
    }
}
