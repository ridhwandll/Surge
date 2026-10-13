// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Core/String.hpp"
#include "AudioID.hpp"

namespace Surge
{
    class Audio;

    class AudioEngine
    {
    public:
        void Initialize();
        void Shutdown();

        // Stops/restarts the audio device
        void Suspend();
        void Resume();

        /*
         * @brief Instantiates a playable sound instance from an Audio asset.
         *
         * Allocates a SoundInstance wrapper and sets up a miniaudio sound under one of two modes:
         * - Streaming (isStreaming = true): Decodes chunks on the fly directly from the encoded
         *   byte buffer. Recommended for long tracks (BGM, ambience, dialogue) to save RAM.
         * - Pre-decoded (isStreaming = false): Fully decodes the file into 16-bit PCM frames in memory and registers it with the s
         *   engine's resource manager. Recommended for short, frequently played sound effects (SFX) to minimize runtime CPU overhead.
         *
         * @param clip             Shared reference to the audio asset containing encoded audio data.
         * @param isStreaming      Whether to stream/decode on the fly vs. play from pre-decoded PCM cache.
         * @param isSpatialized    Whether to enable 3D spatial audio calculations (panning, attenuation).
         *
         * @return AudioID An opaque handle (internally known as SoundInstance*) to the created sound, or `nullptr` on failure.
         *         Must be released via `AudioEngine::DestroyAudio()` when no longer needed.
         */
        AudioID CreateAudio(const Ref<Audio>& clip, bool isStreaming, bool isSpatialized);
        void DestroyAudio(AudioID audioId);
        const Audio* GetClip(AudioID audioId) const; // The clip a sound plays, null for an invalid ID

        void PlayAudio(AudioID audioId);
        void StopAudio(AudioID audioId);

        void SetAttenuationModel(AudioID audioId, AttenuationModel model);
        void SetLooping(AudioID audioId, bool isLooping);
        void SetVolume(AudioID audioId, float volume);
        void SetPitch(AudioID audioId, float pitch);
        void SetMinDistance(AudioID audioId, float distance);
        void SetMaxDistance(AudioID audioId, float distance);
        void SetListenerPosition(float x, float y, float z);
        void SetPosition(float x, float y, float z, AudioID audioId);
        void SeekTo(AudioID audioId, float seconds);

        // Called by ~Audio once no sound plays the clip: drops its decoded data registration
        void UnregisterClip(Audio& clip);

    private:
        bool EnsureDecoded(Audio& clip);
    };
}
