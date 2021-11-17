// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/PlatformMisc.hpp"

namespace Surge
{
    void PlatformMisc::RequestExit()
    {
        SendMessage((HWND)Core::GetWindow()->GetNativeWindowHandle(), WM_QUIT, 0, 0);
    }

    void PlatformMisc::ErrorMessageBox(const char* text)
    {
        MessageBox(NULL, text, "Error!", MB_ICONEXCLAMATION | MB_OK);
    }
} // namespace Surge