// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Asset/Serializer/AssetStamp.hpp"

namespace Surge::AudioBinary
{
    // AudioBinary (.rAUDIO)
    // AssetStamp
    // Payload       the source file (wav/mp3/ogg/flac) byte for byte. The Audio asset keeps only the payload, the AudioEngine plays it from memory

    inline constexpr Uint VERSION = 1;

    bool Write(const String& path, const AssetStamp& stamp, const String& sourceAbsPath);
    bool Read(const String& path, AssetStamp& outStamp, Vector<Byte>& outEncodedData);
}
