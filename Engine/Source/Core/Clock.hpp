// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge::Clock
{
    // Starts the internal timer. Initializes Life and Delta to current time
    SURGE_API void Start();

    // Returns the time (in seconds) since the engine was initialized
    SURGE_API float GetLife();

    // Returns the time (in seconds) since the last frame
    SURGE_API float GetDelta();

    // Updates the internal timer. Increments Life and refreshes Delta
    SURGE_API void Update();
}
