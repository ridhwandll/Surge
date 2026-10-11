// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include <glm/glm.hpp>
#include <cstring>
#include <iterator>

namespace Surge
{
    // Value types a script can expose to the Inspector(Editor only) through its Properties table (see Property.* in SurgeAPI.lua)
    enum class ScriptFieldType : uint8_t
    {
        FLOAT = 0,
        INT,
        BOOL,
        STRING,
        VEC2,
        VEC3,
        VEC4,
        COLOR,  // Vec4 edited with a color picker
        ENTITY, // Reference to another entity of the scene, saved by UUID
        ASSET,  // Reference to an asset, saved by AssetID; the script gets its path (works with SetMesh, Scene.Load, ...)
        ENUM,   // One string out of a fixed list, edited with a dropdown
        ARRAY,  // A Lua array of one element type (any of the above except ENUM and ARRAY)
        COUNT
    };

    struct ScriptFieldValue
    {
        double Number = 0.0;                // FLOAT, INT
        bool Bool = false;                  // BOOL
        String Text;                        // STRING, ENUM
        glm::vec4 Vector = glm::vec4(0.0f); // VEC2, VEC3, VEC4, COLOR (unused components stay 0)
        uint64_t EntityID = 0;              // ENTITY: IDComponent UUID, 0 = none
        uint64_t AssetID = 0;               // ASSET: 0 = none
        ::Surge::Vector<ScriptFieldValue> Elements; // ARRAY (qualified: the Vector member above hides the alias)
    };

    // A value set in the Inspector for one entity, saved with the scene. Written into the script's Properties table once,
    struct ScriptFieldOverride
    {
        String Name;
        ScriptFieldType Type = ScriptFieldType::FLOAT;
        ScriptFieldType ElementType = ScriptFieldType::FLOAT; // For Type = ScriptFieldType::ARRAY only
        ScriptFieldValue Value;
    };

    inline const char* ScriptFieldTypeToString(ScriptFieldType type)
    {
        constexpr const char* names[] = { "FLOAT", "INT", "BOOL", "STRING", "VEC2", "VEC3", "VEC4", "COLOR", "ENTITY", "ASSET", "ENUM", "ARRAY" };
        static_assert(std::size(names) == static_cast<size_t>(ScriptFieldType::COUNT));
        return type < ScriptFieldType::COUNT ? names[static_cast<size_t>(type)] : "";
    }

    inline bool ScriptFieldTypeFromString(const char* str, ScriptFieldType& outType)
    {
        for(uint8_t i = 0; i < static_cast<uint8_t>(ScriptFieldType::COUNT); i++)
        {
            if(std::strcmp(str, ScriptFieldTypeToString(static_cast<ScriptFieldType>(i))) == 0)
            {
                outType = static_cast<ScriptFieldType>(i);
                return true;
            }
        }
        return false;
    }

} // namespace Surge
