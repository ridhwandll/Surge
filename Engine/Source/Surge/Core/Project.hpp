// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "String.hpp"
#include "Surge/Core/Path.hpp"
#include "Surge/Asset/Asset.hpp"

namespace Surge
{
    struct Project
    {
        String Name;
        String Version;
        AssetID StartScene = AssetID::INVALID;

        bool IsValid() const { return !Name.empty(); }
        void Clear() { Name.clear(); Version.clear(); StartScene = AssetID(AssetID::INVALID); }

        // <Name>.surgeproj (JSON)
        bool Save(const Path& path) const;
        bool Load(const Path& path); // Throws on malformed JSON
    };
}
