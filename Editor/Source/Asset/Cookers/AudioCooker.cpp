// Copyright (c) - SurgeTechnologies - All rights reserved
#include "AudioCooker.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Core/Core.hpp"

namespace Surge
{
    CookResult AudioCooker::Cook(const String& sourceAbsPath, AssetID id) const
    {
        CookResult result;
        AssetManager* am = Core::GetAssetManager();
        const String outputPath = am->GetSidecarPath(id, AssetType::AUDIO);

        // The encoded file is kept as is behind the stamp, AudioEngine decodes/streams it at runtime
        result.Success = AudioBinary::Write(outputPath, AssetStampWriter::Build(sourceAbsPath, GetCookerVersion()), sourceAbsPath);
        result.OutputPath = outputPath;
        result.InputMegaBytes = Filesystem::FileSizeInMB(sourceAbsPath);
        result.OutputMegaBytes = Filesystem::FileSizeInMB(outputPath);
        return result;
    }
}
