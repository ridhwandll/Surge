// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"

namespace Surge::PlatformMisc
{
    std::string GetPersistantStoragePath();
    void RequestExit();
    void ErrorMessageBox(const char* text);

} // namespace Surge::PlatformMisc