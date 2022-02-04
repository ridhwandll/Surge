// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Project/Project.hpp"

namespace Surge
{
    struct PersistantProjectData
    {
        String Name;
        String AbsolutePath;
        String CreationDateString;
        bool Valid = false;
        bool StartupProject = false;
    };

    class ProjectBrowserWindow
    {
    public:
        ProjectBrowserWindow() = default;
        ~ProjectBrowserWindow() = default;
        void Initialize();
        void Shutdown();

        void Render();
        void SetProjectLaunchCallback(std::function<void(const ProjectMetadata& metadata)> func) { mOnProjectLaunch = func; }

    private:
        void LaunchProject(const PersistantProjectData& projData);
        void WriteProjectsToPersistantStorage();
        void LoadProjectsFromPersistantStorage();
        void RemoveProjectFromPersistantStorage(Uint index);

    private:
        String mProjectNameBuffer = "MyProject";
        String mSceneNameBuffer = "MainScene";
        String mProjectPathBuffer;

        Path mPersistantStoragePath;
        std::function<void(const ProjectMetadata& metadata)> mOnProjectLaunch;
        Vector<PersistantProjectData> mPersistantProjectData;
    };

} // namespace Surge
