// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Audio/AudioEngine.hpp"
#include "Audio.hpp"
#include <miniaudio/miniaudio.h>
#include <glm/common.hpp>
#include <format>

namespace Surge
{
    // What an AudioID(a void* under the hood) points to
    struct SoundInstance
    {
        ma_sound Sound;
        ma_decoder Decoder; // Streaming only: decodes the clip's bytes while the sound plays
        bool HasDecoder = false;
        Ref<Audio> Clip;
    };

    static ma_engine* sAudioEngine = nullptr;
    static Uint sClipCounter = 0;

    static FORCEINLINE ma_sound* GetSound(AudioID audioId)
    {
        return &static_cast<SoundInstance*>(audioId)->Sound;
    }

    void AudioEngine::Initialize()
    {
        sAudioEngine = new ma_engine();
        ma_engine_config config = ma_engine_config_init();

        const ma_result result = ma_engine_init(&config, sAudioEngine);
        if(result != MA_SUCCESS)
            Log<Severity::Error>("[AudioEngine] Failed to initialize miniaudio! ({})", static_cast<int>(result));

        Log<Severity::Info>("AudioEngine initialized");
    }

    void AudioEngine::Shutdown()
    {
        ma_engine_uninit(sAudioEngine);
        delete sAudioEngine;
        sAudioEngine = nullptr;
    }

    void AudioEngine::Suspend()
    {
        if(ma_engine_stop(sAudioEngine) != MA_SUCCESS)
            Log<Severity::Warn>("[AudioEngine] Failed to suspend the audio device");
    }

    void AudioEngine::Resume()
    {
        if(ma_engine_start(sAudioEngine) != MA_SUCCESS)
            Log<Severity::Warn>("[AudioEngine] Failed to resume the audio device");
    }

    bool AudioEngine::EnsureDecoded(Audio& clip)
    {
        if(!clip.mRegisteredName.empty())
            return true;

        // 16 bit PCM at the file's own channel count/rate, miniaudio converts while mixing
        const Vector<Byte>& encoded = clip.GetEncodedData();
        ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_s16, 0, 0);
        ma_decoder decoder;
        if(ma_decoder_init_memory(encoded.data(), encoded.size(), &decoderConfig, &decoder) != MA_SUCCESS)
            return false;

        ma_format format;
        ma_uint32 channels = 0;
        ma_uint32 sampleRate = 0;
        ma_decoder_get_data_format(&decoder, &format, &channels, &sampleRate, nullptr, 0);

        ma_uint64 lengthInFrames = 0;
        if(ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames) == MA_SUCCESS && lengthInFrames > 0)
            clip.mDecodedFrames.reserve(static_cast<size_t>(lengthInFrames * channels));

        // Chunked: the length is only an estimate for some formats (mp3)
        constexpr ma_uint64 CHUNK_FRAMES = 4096;
        for EVER
        {
            const size_t offset = clip.mDecodedFrames.size();
            clip.mDecodedFrames.resize(offset + static_cast<size_t>(CHUNK_FRAMES * channels));

            ma_uint64 framesRead = 0;
            const ma_result result = ma_decoder_read_pcm_frames(&decoder, clip.mDecodedFrames.data() + offset, CHUNK_FRAMES, &framesRead);
            clip.mDecodedFrames.resize(offset + static_cast<size_t>(framesRead * channels));
            if(result != MA_SUCCESS || framesRead < CHUNK_FRAMES)
                break;
        }
        ma_decoder_uninit(&decoder);

        if(clip.mDecodedFrames.empty() || channels == 0)
        {
            clip.mDecodedFrames.clear();
            return false;
        }

        const String name = std::format("surge-audio:{}", ++sClipCounter);
        const ma_uint64 frameCount = clip.mDecodedFrames.size() / channels;
        if(ma_resource_manager_register_decoded_data(ma_engine_get_resource_manager(sAudioEngine), name.c_str(), clip.mDecodedFrames.data(), frameCount, ma_format_s16, channels, sampleRate) != MA_SUCCESS)
        {
            clip.mDecodedFrames.clear();
            return false;
        }

        clip.mRegisteredName = name;
        return true;
    }

    void AudioEngine::UnregisterClip(Audio& clip)
    {
        if(clip.mRegisteredName.empty())
            return;

        if(sAudioEngine)
            ma_resource_manager_unregister_data(ma_engine_get_resource_manager(sAudioEngine), clip.mRegisteredName.c_str());
        clip.mRegisteredName.clear();
    }

    AudioID AudioEngine::CreateAudio(const Ref<Audio>& clip, bool isStreaming, bool isSpatialized)
    {
        if(!clip || clip->GetEncodedData().empty() || !sAudioEngine)
            return nullptr;

        SoundInstance* instance = new SoundInstance();
        instance->Clip = clip;
        const ma_uint32 flags = isSpatialized ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;

        ma_result result = MA_ERROR;
        if(isStreaming)
        {
            const Vector<Byte>& encoded = clip->GetEncodedData();
            result = ma_decoder_init_memory(encoded.data(), encoded.size(), nullptr, &instance->Decoder);
            if(result == MA_SUCCESS)
            {
                instance->HasDecoder = true;
                result = ma_sound_init_from_data_source(sAudioEngine, &instance->Decoder, flags, nullptr, &instance->Sound);
            }
        }
        else if(EnsureDecoded(*instance->Clip)) // clip->mRegisteredName: Reistered with miniaudio's resource manager
            result = ma_sound_init_from_file(sAudioEngine, clip->mRegisteredName.c_str(), flags, nullptr, nullptr, &instance->Sound);

        if(result != MA_SUCCESS)
        {
            Log<Severity::Error>("[AudioEngine] Failed to create a sound for audio asset {} ({})", clip->GetID().Get(), static_cast<int>(result));
            if(instance->HasDecoder)
                ma_decoder_uninit(&instance->Decoder);
            delete instance;
            return nullptr;
        }
        return static_cast<AudioID>(instance);
    }

    void AudioEngine::DestroyAudio(AudioID audioId)
    {
        if(!audioId)
            return;

        SoundInstance* instance = static_cast<SoundInstance*>(audioId);
        ma_sound_uninit(&instance->Sound);
        if(instance->HasDecoder)
            ma_decoder_uninit(&instance->Decoder);
        delete instance; // Last Ref to the clip: ~Audio unregisters it
    }

    const Audio* AudioEngine::GetClip(AudioID audioId) const
    {
        return audioId ? static_cast<SoundInstance*>(audioId)->Clip.Raw() : nullptr;
    }

    void AudioEngine::PlayAudio(AudioID audioId)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_start(GetSound(audioId));
    }

    void AudioEngine::StopAudio(AudioID audioId)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_stop(GetSound(audioId));
    }

    void AudioEngine::SetAttenuationModel(AudioID audioId, AttenuationModel model)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_attenuation_model(GetSound(audioId), (ma_attenuation_model)model);
    }

    void AudioEngine::SetLooping(AudioID audioId, bool isLooping)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_looping(GetSound(audioId), isLooping);
    }

    void AudioEngine::SetVolume(AudioID audioId, float volume)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_volume(GetSound(audioId), glm::clamp(volume, 0.0f, 2.0f));
    }

    void AudioEngine::SetPitch(AudioID audioId, float pitch)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_pitch(GetSound(audioId), glm::clamp(pitch, 0.1f, 3.0f));
    }

    void AudioEngine::SetMinDistance(AudioID audioId, float distance)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_min_distance(GetSound(audioId), distance);
    }

    void AudioEngine::SetMaxDistance(AudioID audioId, float distance)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound_set_max_distance(GetSound(audioId), distance);
    }

    void AudioEngine::SetListenerPosition(float x, float y, float z)
    {
        ma_engine_listener_set_position(sAudioEngine, 0, x, y, z);
    }

    void AudioEngine::SetPosition(float x, float y, float z, AudioID audioId)
    {
        if(audioId)
            ma_sound_set_position(GetSound(audioId), x, y, z);
        else
            Log<Severity::Warn>("[AudioEngine::SetPosition] Invalid AudioID!");
    }

    void AudioEngine::SeekTo(AudioID audioId, float seconds)
    {
        SG_ASSERT(audioId, "AudioID is NULL!");
        ma_sound* sound = GetSound(audioId);
        ma_uint32 sampleRate;
        ma_result result = ma_sound_get_data_format(sound, NULL, NULL, &sampleRate, NULL, 0);

        if(result == MA_SUCCESS)
        {
            ma_uint64 frameIndex = static_cast<ma_uint64>(seconds * sampleRate);
            ma_sound_seek_to_pcm_frame(sound, frameIndex);
        }
        else
            Log<Severity::Warn>("[AudioEngine::SeekTo] Failed to query data format. Cannot seek!");
    }
}
