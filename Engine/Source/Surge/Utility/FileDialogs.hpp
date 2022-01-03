// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge::FileDialog
{
    SURGE_API String OpenFile(const char* filter);
    SURGE_API String SaveFile(const char* filter);
    SURGE_API String ChooseFolder();

} // namespace Surge::FileDialog
