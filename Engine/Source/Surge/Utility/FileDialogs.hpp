// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge::FileDialog
{
    String OpenFile(const char* filter);
    String SaveFile(const char* filter);
    String ChooseFolder();

} // namespace Surge::FileDialog
