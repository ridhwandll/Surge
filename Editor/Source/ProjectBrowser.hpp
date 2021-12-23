// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"

namespace Surge
{
    struct PersistantProjectData
    {
        String Name;
        String AbsolutePath;
    };

    class ProjectBrowserWindow
    {
    public:
        ProjectBrowserWindow();
        ~ProjectBrowserWindow() = default;

        void Render();

    private:
        void LaunchProject(const String& name, const String& sceneName, const Path& path);
        void SerializeProject();
        void LoadProjectsFromPersistantStorage();
        void RemoveProjectFromPersistantStorage(Uint index);

    private:
        String mProjectNameBuffer = "MyProject";
        String mSceneNameBuffer = "MainScene";
        String mProjectPathBuffer;
        Path mPersistantStoragePath;

        Vector<PersistantProjectData> mPersistantProjectData;
    };

} // namespace Surge
