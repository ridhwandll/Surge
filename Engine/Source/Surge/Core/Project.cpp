// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Project.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <json/json.hpp>

namespace Surge
{
    bool Project::Save([[maybe_unused]] const Path& path) const
    {
#ifdef SURGE_PLATFORM_ANDROID
        Log<Severity::Error>("[Project] Save is unsupported on Android runtime. APK assets are readonly!");
        return false;
#else
        nlohmann::json outJson;
        outJson["Project"]["Name"] = Name;
        outJson["Project"]["Version"] = Version;
        outJson["Project"]["StartScene"] = StartScene.Get(); // Not static_cast: on a const UUID it goes through operator bool and writes 1

        if(!Filesystem::WriteTextFile(path, outJson.dump(4)))
        {
            Log<Severity::Error>("[Project] Failed to write to '{}'", path.string());
            return false;
        }
        return true;
#endif
    }

    bool Project::Load(const Path& path)
    {
        String jsonContents;
        if(!Filesystem::ReadTextFile(path, jsonContents))
        {
            Log<Severity::Error>("[Project] Failed to read file '{}'", path.string());
            return false;
        }

        nlohmann::json parsedJson = nlohmann::json::parse(jsonContents);
        if(parsedJson.is_discarded())
        {
            Log<Severity::Error>("[Project] Corrupt or invalid JSON at '{}'", path.string());
            return false;
        }

        if(!parsedJson.contains("Project"))
        {
            Log<Severity::Error>("[Project] Corrupted file '{}'", path.string());
            return false;
        }

        const auto& projNode = parsedJson["Project"];
        Name = projNode.value("Name", "");
        Version = projNode.value("Version", "1.0");
        StartScene = static_cast<AssetID>(projNode.value("StartScene", 0ULL));
        return true;
    }
}
