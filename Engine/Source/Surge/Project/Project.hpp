// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/ECS/Scene.hpp"

namespace Surge
{
    enum class ProjectState
    {
        Edit,
        Play
    };

    struct ProjectDirectoryData
    {
        Path InternalDirectory; //Path to the .surge folder
        Path ProjectFilePath;   // Path to the .surgeProj file
    };

    // The idea behing Project is:-
    // - It will contain settings for the engine subsystems(RenderEngine, ScriptEngine, PhysicsEngine, AudioEngine etc.)
    // - A Project can have many scenes, more like a scene container
    // - Contains the ordered array of Scenes to play, manage scene transitions
    // TODO: Add Support for re-ordering scenes

    class Project
    {
    public:
        Project(const String& name, const Path& path);
        ~Project();

        void OnRuntimeStart();
        void Update(EditorCamera& camera);
        void OnRuntimeEnd();

        Ref<Scene> AddScene(const String& name);
        void RemoveScene(Uint arrayIndex);
        Ref<Scene>& GetScene(Uint arrayIndex);
        const String& GetName() const { return mName; }
        const UUID& GetUUID() const { return mProjectID; }

        Ref<Scene>& GetActiveScene() { return mScenes[mActiveSceneIndex]; }
        Uint& GetActiveSceneIndex() { return mActiveSceneIndex; }
        void SetActiveScene(Uint sceneindex);
        void AddActiveSceneChangeCallback(std::function<void(Ref<Scene>& scene)> func) { mOnActiveSceneChangeCallbacks.push_back(func); }

        const ProjectState& GetState() const { return mProjectState; }
        void SetState(ProjectState state) { mProjectState = state; }

        auto& GetAllScenes() { return mScenes; }
        auto& GetAllRuntimeScenes() { return mRuntimeSceneStorage; }

        operator bool() { return mIsValid; }
        bool operator==(const Project& other) const { return mProjectID == other.mProjectID; }
        bool operator!=(const Project& other) const { return !(*this == other); }

    private:
        String mName;
        Path mPath;
        UUID mProjectID;
        ProjectState mProjectState;
        Uint mActiveSceneIndex;

        ProjectDirectoryData mDirectoryData;
        bool mIsValid;
        Vector<Ref<Scene>> mScenes;
        Vector<Ref<Scene>> mRuntimeSceneStorage;
        Vector<std::function<void(Ref<Scene>& scene)>> mOnActiveSceneChangeCallbacks;
    };

} // namespace Surge
