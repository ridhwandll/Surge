// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/PlatformMisc.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <ShlObj_core.h>

namespace Surge
{
    static bool sPersistantDirectoryExists = false;
    std::string PlatformMisc::GetPersistantStoragePath()
    {
        String resultantPath;
        PWSTR roamingFilePath;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &roamingFilePath);
        SG_ASSERT_NOMSG(result == S_OK);
        std::wstring filepath = roamingFilePath;
        std::replace(filepath.begin(), filepath.end(), L'\\', L'/');
        std::transform(filepath.begin(), filepath.end(), std::back_inserter(resultantPath), [](wchar_t c) { return (char)c; });
        resultantPath += "/Surge Engine";

        if (!sPersistantDirectoryExists)
            sPersistantDirectoryExists = Filesystem::CreateOrEnsureDirectory(resultantPath);
        return resultantPath;
    }

    void PlatformMisc::RequestExit()
    {
        SendMessage(static_cast<HWND>(Core::GetWindow()->GetNativeWindowHandle()), WM_QUIT, 0, 0);
    }

    void PlatformMisc::ErrorMessageBox(const char* text)
    {
        MessageBox(NULL, text, "Error!", MB_ICONERROR | MB_OK);
    }

    glm::vec2 PlatformMisc::GetScreenSize()
    {
        return glm::vec2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

} // namespace Surge
