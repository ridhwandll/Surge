// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Audio.hpp"
#include "Surge/Audio/AudioEngine.hpp"
#include "Surge/Core/Core.hpp"

namespace Surge
{
    Audio::~Audio()
    {
        // No sound plays this clip anymore (each one holds a Ref), drop the registration before the decoded frames go
        if(!mRegisteredName.empty())
        {
            if(AudioEngine* audioEngine = Core::GetAudioEngine())
                audioEngine->UnregisterClip(*this);
        }
    }
}
