// Copyright (c) - SurgeTechnologies - All rights reserved
#include "AudioBinaryFormat.hpp"
#include "Surge/Asset/Serializer/BinaryHelpers.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge::AudioBinary
{
    bool Write(const String& path, const AssetStamp& stamp, const String& sourceAbsPath)
    {
        Vector<Byte> source;
        if(!Filesystem::ReadBinaryFile(sourceAbsPath, source) || source.empty())
        {
            Log<Severity::Error>("[AudioBinary] Failed to read {}", sourceAbsPath);
            return false;
        }

        Vector<Byte> buffer;
        buffer.reserve(sizeof(AssetStamp) + source.size());
        WriteData(buffer, stamp);
        buffer.insert(buffer.end(), source.begin(), source.end());

        if(!Filesystem::WriteBinaryFile(path, buffer.data(), buffer.size()))
        {
            Log<Severity::Error>("[AudioBinary] Failed to write {}", path);
            return false;
        }
        Log<Severity::Trace>("[AudioBinary] Cooked {}", path);
        return true;
    }

    bool Read(const String& path, AssetStamp& outStamp, Vector<Byte>& outEncodedData)
    {
        Vector<Byte> fileData;
        if(!Filesystem::ReadBinaryFile(path, fileData))
        {
            Log<Severity::Error>("[AudioBinary] Failed to read {}", path);
            return false;
        }

        BinaryReader reader(fileData.data(), fileData.size());
        reader.Read(outStamp);
        if(reader.Failed || outStamp.Magic != AssetStamp::kMagic || reader.Remaining() == 0)
        {
            Log<Severity::Error>("[AudioBinary] {} is not cooked audio. Re-cook it in the Editor (or export the game again)", path);
            return false;
        }

        // Drop the stamp in place: the payload keeps the file's allocation instead of being copied
        fileData.erase(fileData.begin(), fileData.begin() + sizeof(AssetStamp));
        outEncodedData = std::move(fileData);
        return true;
    }
}
