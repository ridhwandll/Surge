// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"

namespace Surge::Clock
{
    // Starts the internal timer. Initializes Life and Delta to current time
    void Start();

    // Returns the time (in seconds) since the engine was initialized
    float GetLife();

    // Returns the time (in seconds) since the last frame
    float GetDelta();

    // Updates the internal timer. Increments Life and refreshes Delta
    void Update();
} // namespace Surge::Clock
