// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Core/Path.hpp"
#include <cstdint>

namespace Surge
{
    struct RecentProject
    {
        String Name;
        String Filepath;  // Path of the .surgeproj file
        String Directory; // Folder holding the project file, display only
        String Version;   // Read from the project file on Refresh()
        int64_t LastOpened = 0; // Unix time, seconds
        bool Pinned = false;
        bool Missing = false; // The project file was not found on the last Refresh()
    };

    class Event;
    class AssetManager;
    struct Project;
    class ProjectBrowser
    {
    public:
        void Init();
        void Render(float titleBarHeight);
        void OnEvent(Event& e); // .surgeproj files (or folders holding one) dropped on the window are opened
        void Refresh();         // Re-checks the recent projects on disk, the Editor calls it when a project gets closed

    private:
        enum class Page
        {
            PROJECTS,
            NEW_PROJECT
        };

        enum class SortMode
        {
            LAST_OPENED,
            NAME
        };

        void RenderSidebar(float height);
        void RenderProjectsPage();
        void RenderProjectList();
        void RenderEmptyState(bool noMatches);
        void RenderNewProjectPage();
        void RenderErrorBanner(float width);
        void HandleShortcuts();

        void OpenNewProjectPage();
        void RebuildVisibleList();
        void MoveSelection(int delta);
        void ValidateNewProject();

        void CreateProject();
        void OpenProject(const String& filepath);
        void BrowseForProject();
        void LocateProject(size_t index);
        void RemoveRecentProject(size_t index);
        void ActivateAssetManager(const Path& projectFile);
        void AddRecentProject(const Project& project, const String& filepath);
        int FindRecentProject(const String& filepath) const;
        void SaveRecentProjects();
        void LoadRecentProjects();
        void SetError(String message);

    private:
        AssetManager* mAssetManager = nullptr;

        Vector<RecentProject> mRecentProjects; // Most recently opened first
        Vector<uint32_t> mVisible;             // Indices into mRecentProjects, filtered by the search and sorted (pinned first)
        String mSelectedPath;
        String mDroppedProject; // Opened on the next Render, never from inside the event callback
        String mLastLocation;   // Last folder a project was created in
        String mError;

        char mSearchBuffer[128] = {};
        char mNameBuffer[128] = {};
        char mLocationBuffer[512] = {};

        // New project validation, recomputed only when the name/location change
        String mValidationMessage; // Empty when the project can be created
        Path mNewProjectFile;
        bool mNameError = false;
        bool mValidationDirty = true;

        Page mPage = Page::PROJECTS;
        SortMode mSortMode = SortMode::LAST_OPENED;
        bool mVisibleDirty = true;
        bool mFocusSearch = false;
        bool mFocusName = false;
        bool mScrollToSelected = false;
    };

} // namespace Surge
