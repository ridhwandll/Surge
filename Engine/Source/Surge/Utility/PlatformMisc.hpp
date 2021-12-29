// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <glm/glm.hpp>

namespace Surge::PlatformMisc
{
    std::string GetPersistantStoragePath();
    void RequestExit();
    void ErrorMessageBox(const char* text);
    glm::vec2 GetScreenSize();

} // namespace Surge::PlatformMisc