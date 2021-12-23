// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Project.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Serializer/Serializer.hpp"

namespace Surge
{
    Project::Project(const String& name, const Path& path)
    {
        mName = name;
        mPath = path;
        mProjectID = UUID();
        mProjectState = ProjectState::Edit;
        mActiveSceneIndex = 0;

        mDirectoryData.InternalDirectory = fmt::format("{0}/{1}", path, ".surge");

        bool success;
        success = Filesystem::CreateOrEnsureDirectory(mDirectoryData.InternalDirectory);
        SG_ASSERT(success, "Cannot create directory!");

        mDirectoryData.ProjectFilePath = fmt::format("{0}/{1}.surgeProj", mDirectoryData.InternalDirectory, name);
        Filesystem::CreateOrEnsureFile(mDirectoryData.ProjectFilePath);
        mIsValid = true;
    }

    Project::~Project()
    {
        mIsValid = false;
        mOnActiveSceneChangeCallbacks.clear();
        mScenes.clear();
    }

    void Project::OnRuntimeStart()
    {
        mRuntimeSceneStorage.resize(mScenes.size());
        for (Uint i = 0; i < mScenes.size(); i++)
        {
            // Create the runtime scene
            auto& runtimeScene = mRuntimeSceneStorage[i];
            runtimeScene = Ref<Scene>::Create(mScenes[i]->GetName(), true);
            mScenes[i]->CopyTo(runtimeScene.Raw()); // Copy the scene
            runtimeScene->OnRuntimeStart();
        }
        Core::GetRenderer()->SetSceneContext(mRuntimeSceneStorage[mActiveSceneIndex]);
    }

    void Project::Update(EditorCamera& camera)
    {
        switch (mProjectState)
        {
            case Surge::ProjectState::Edit:
                mScenes[mActiveSceneIndex]->Update(camera);
                break;
            case Surge::ProjectState::Play:
                mRuntimeSceneStorage[mActiveSceneIndex]->Update();
                break;
        }
    }

    void Project::OnRuntimeEnd()
    {
        for (auto& scene : mRuntimeSceneStorage)
        {
            scene->OnRuntimeEnd();
        }
        mRuntimeSceneStorage.clear();
        Core::GetRenderer()->SetSceneContext(mScenes[mActiveSceneIndex]);
    }

    Ref<Scene> Project::AddScene(const String& name)
    {
        Ref<Scene> newScene = Ref<Scene>::Create(name, false);
        mScenes.push_back(newScene);
        return newScene;
    }

    void Project::RemoveScene(Uint arrayIndex)
    {
        mScenes.erase(mScenes.begin() + arrayIndex);
    }

    Ref<Scene>& Project::GetScene(Uint arrayIndex)
    {
        return mScenes[arrayIndex];
    }

    void Project::SetActiveScene(Uint sceneindex)
    {
        Serializer::Serialize<Project>(mDirectoryData.ProjectFilePath, this);
        Ref<Scene>& activatedScene = GetScene(sceneindex);
        for (auto& func : mOnActiveSceneChangeCallbacks)
            func(activatedScene);

        mActiveSceneIndex = sceneindex;
    }

} // namespace Surge
