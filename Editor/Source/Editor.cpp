// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Editor.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Panels/ViewportPanel.hpp"
#include "Panels/PerformancePanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Panels/InspectorPanel.hpp"
#include <imgui_internal.h>

namespace Surge
{
    void Editor::OnInitialize()
    {
        mRenderer = Core::GetRenderer();
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mCamera.SetActive(true);

        mEditorScene = Ref<Scene>::Create(false);
        Serializer::Deserialize<Scene>("Engine/Assets/Scenes/Test.surge", mEditorScene);

        // Configure panels
        mTitleBar = Titlebar();
        SceneHierarchyPanel* sceneHierarchy;
        sceneHierarchy = mPanelManager.PushPanel<SceneHierarchyPanel>();
        sceneHierarchy->SetSceneContext(mEditorScene.Raw());
        mPanelManager.PushPanel<InspectorPanel>()->SetHierarchy(sceneHierarchy);
        mPanelManager.PushPanel<PerformancePanel>();
        mPanelManager.PushPanel<ViewportPanel>();
        mTitleBar.OnInit();
    }

    void Editor::OnUpdate()
    {
        Resize();
        if (mSceneState == SceneState::Edit)
        {
            mCamera.OnUpdate();
            mEditorScene->Update(mCamera);
        }
        else if (mSceneState == SceneState::Play)
            mRuntimeScene->Update();
    }

    void Editor::OnImGuiRender()
    {
        mTitleBar.Render();
        ImGuiAux::DockSpace();
        mPanelManager.RenderAll();
    }

    void Editor::OnEvent(Event& e)
    {
        mCamera.OnEvent(e);
        mPanelManager.OnEvent(e);
    }

    void Editor::OnRuntimeStart()
    {
        mRuntimeScene = Ref<Scene>::Create(true);
        mEditorScene->CopyTo(mRuntimeScene.Raw());
        mRuntimeScene->OnRuntimeStart();
        mPanelManager.GetPanel<SceneHierarchyPanel>()->SetSceneContext(mRuntimeScene.Raw());
        mSceneState = SceneState::Play;
    }

    void Editor::OnRuntimeEnd()
    {
        SG_ASSERT_NOMSG(mRuntimeScene);
        mRuntimeScene->OnRuntimeEnd();
        mRuntimeScene.Reset();
        mPanelManager.GetPanel<SceneHierarchyPanel>()->SetSceneContext(mEditorScene.Raw());
        mSceneState = SceneState::Edit;
    }

    void Editor::Resize()
    {
        ViewportPanel* viewportPanel = mPanelManager.GetPanel<ViewportPanel>();
        glm::vec2 viewportSize = viewportPanel->GetViewportSize();
        Ref<Framebuffer> framebuffer = mRenderer->GetFinalPassFramebuffer();

        if (mSceneState == SceneState::Play && mRuntimeScene && mRuntimeScene->GetMainCameraEntity().Data1 && mRuntimeScene->GetMainCameraEntity().Data1->GetAspectRatio() != (viewportSize.x / viewportSize.y))
            mRuntimeScene->OnResize(viewportSize.x, viewportSize.y);

        if (FramebufferSpecification spec = framebuffer->GetSpecification();
            viewportSize.x > 0.0f && viewportSize.y > 0.0f &&
            (spec.Width != viewportSize.x || spec.Height != viewportSize.y))
        {
            framebuffer->Resize((Uint)viewportSize.x, (Uint)viewportSize.y);
            mCamera.SetViewportSize(viewportSize);

            if (mSceneState == SceneState::Edit)
                mEditorScene->OnResize(viewportSize.x, viewportSize.y);
        }
    }

    void Editor::OnShutdown()
    {
    }
} // namespace Surge

// Entry point
int main()
{
    Surge::ClientOptions clientOptions;
    clientOptions.EnableImGui = true;
    clientOptions.WindowDescription = {1280, 720, "Surge Editor", Surge::WindowFlags::CreateDefault | Surge::WindowFlags::EditorAcceleration};

    Surge::Editor* app = Surge::MakeClient<Surge::Editor>();
    app->SetOptions(clientOptions);

    Surge::Core::Initialize(app);
    Surge::Core::Run();
    Surge::Core::Shutdown();
}