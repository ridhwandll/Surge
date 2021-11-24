// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/PlatformMisc.hpp"

namespace Surge
{
    void PlatformMisc::RequestExit()
    {
        SendMessage(static_cast<HWND>(Core::GetWindow()->GetNativeWindowHandle()), WM_QUIT, 0, 0);
    }

    void PlatformMisc::ErrorMessageBox(const char* text)
    {
        MessageBox(NULL, text, "Error!", MB_ICONERROR | MB_OK);
    }

} // namespace Surge
