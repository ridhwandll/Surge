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

    struct SURGE_API ProjectMetadata
    {
        ProjectMetadata() = default;
        ProjectMetadata(const String& name, const Path& path);
        String Name;              // Name of the project, what else
        Path ProjPath;            // Path to the project folder
        Path InternalDirectory;   // Path to the .surge folder   -> (ProjPath/.surge)
        Path ProjectMetadataPath; // Path to the .surgeProj file -> (ProjPath/.surge/Example.surgeProj)
        UUID ProjectID;           // Unique Project ID
        Uint ActiveSceneIndex;    // The index of the scene which is active (Explanation++)

        Vector<SceneMetadata> SceneMetadatas; // "Relative" Path to the scene
    };

    // The idea behing Project is:-
    // - It will contain settings for the engine subsystems(RenderEngine, ScriptEngine, PhysicsEngine, AudioEngine etc.)
    // - A Project can have many scenes, more like a scene container
    // - Contains the ordered array of Scenes to play, manage scene transitions
    // TODO: Add Support for re-ordering scenes

    class SURGE_API Project
    {
    public:
        Project();
        ~Project();

        void Invalidate(const String& name, const Path& path);
        void Invalidate(const ProjectMetadata& metadata);
        void Destroy();

        void OnRuntimeStart();
        void Update(EditorCamera& camera);
        void OnRuntimeEnd();

        Ref<Scene> AddScene(const String& name, const Path& path);
        Ref<Scene> AddScene(const SceneMetadata& metadata);
        void RenameScene(Uint index, const String& newName);
        void RemoveScene(Uint arrayIndex);

        Ref<Scene>& GetScene(Uint arrayIndex);
        Scene* GetActiveScene()
        {
            switch (mProjectState)
            {
                case Surge::ProjectState::Edit:
                    return mScenes[mMetadata.ActiveSceneIndex].Raw();
                case Surge::ProjectState::Play:
                    return mRuntimeSceneStorage[mMetadata.ActiveSceneIndex].Raw();
            }
            return nullptr;
        }
        void SetActiveScene(Uint sceneindex);
        void AddActiveSceneChangeCallback(std::function<void(Ref<Scene>& scene)> func) { mOnActiveSceneChangeCallbacks.push_back(func); }

        const ProjectMetadata& GetMetadata() const { return mMetadata; }
        const ProjectState& GetState() const { return mProjectState; }
        void SetState(ProjectState state) { mProjectState = state; }
        void Save();

        auto& GetAllScenes() { return mScenes; }
        auto& GetAllRuntimeScenes() { return mRuntimeSceneStorage; }

        operator bool() { return mIsValid; }
        bool operator==(const Project& other) const { return mMetadata.ProjectID == other.mMetadata.ProjectID; }
        bool operator!=(const Project& other) const { return !(*this == other); }

    private:
        ProjectState mProjectState = ProjectState::Edit;

        ProjectMetadata mMetadata;

        bool mIsValid = false;
        Vector<Ref<Scene>> mScenes;
        Vector<Ref<Scene>> mRuntimeSceneStorage;
        Vector<std::function<void(Ref<Scene>& scene)>> mOnActiveSceneChangeCallbacks;
    };

} // namespace Surge
