// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UILayoutCooker.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Asset/Serializer/UILayout/UILayoutSerializer.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge
{
    CookResult UILayoutCooker::Cook(const String& sourceAbsPath, AssetID id) const
    {
        AssetManager* am = Core::GetAssetManager();
        const String sidecarPath = am->GetSidecarPath(id, AssetType::UI_LAYOUT);

        CookResult result;
        result.OutputPath = sidecarPath;

        String json;
        Ref<UILayout> layout = UILayout::Create();
        if(!Filesystem::ReadTextFile(sourceAbsPath, json) || !UILayoutSerializer::FromJson(json, *layout))
        {
            Log<Severity::Error>("[UILayoutCooker] Failed to read UI layout: {}", sourceAbsPath);
            return result;
        }

        result.Success = UILayoutBinary::Write(sidecarPath, AssetStampWriter::Build(sourceAbsPath, GetCookerVersion()), *layout);
        result.InputMegaBytes = Filesystem::FileSizeInMB(sourceAbsPath);
        result.OutputMegaBytes = Filesystem::FileSizeInMB(sidecarPath);
        return result;
    }
}
