// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"

namespace Surge
{
    // An audio clip: the source file (wav/mp3/ogg/flac) exactly as cooked, played from memory by the AudioEngine.
    // Streaming sounds decode these bytes as they play; the other sounds share PCM the AudioEngine decodes once, on first use
    class Audio : public Asset
    {
    public:
        explicit Audio(Vector<Byte>&& encodedData)
            : mEncodedData(std::move(encodedData)) {}
        ~Audio() override;

        SURGE_ASSET_TYPE(AssetType::AUDIO);

        const Vector<Byte>& GetEncodedData() const { return mEncodedData; }

    private:
        Vector<Byte> mEncodedData;

        // Owned by the AudioEngine: decoded once for the non-streaming sounds, registered with miniaudio under mRegisteredName
        Vector<int16_t> mDecodedFrames;
        String mRegisteredName;

        friend class AudioEngine;
    };
}
