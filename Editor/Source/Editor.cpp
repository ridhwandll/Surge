// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Editor.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Panels/ViewportPanel.hpp"
#include "Panels/PerformancePanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Panels/InspectorPanel.hpp"
#include "Panels/RenderProcedurePanel.hpp"
#include "Panels/ProjectSettingsPanel.hpp"

namespace Surge
{
    void Editor::OnInitialize()
    {
        mRenderer = Core::GetRenderer();
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mCamera.SetActive(true);

        mActiveProject = Project("SandboxDemo");
        Ref<Scene> scene = mActiveProject.AddScene("Test");
        Serializer::Deserialize<Scene>("Engine/Assets/Scenes/Test.surge", scene);

        // Configure panels
        mTitleBar = Titlebar();
        SceneHierarchyPanel* sceneHierarchy;
        sceneHierarchy = mPanelManager.PushPanel<SceneHierarchyPanel>();
        sceneHierarchy->SetSceneContext(mActiveProject.GetActiveScene().Raw());

        mPanelManager.PushPanel<InspectorPanel>()->SetHierarchy(sceneHierarchy);
        mPanelManager.PushPanel<PerformancePanel>();
        ViewportPanel* viewport = mPanelManager.PushPanel<ViewportPanel>();
        mPanelManager.PushPanel<RenderProcedurePanel>();
        mPanelManager.PushPanel<ProjectSettingsPanel>();
        mTitleBar.OnInit();

        mRenderer->SetSceneContext(mActiveProject.GetActiveScene());
        mRenderer->SetRenderArea(viewport->GetViewportSize().x, viewport->GetViewportSize().y);

        mActiveProject.AddActiveSceneChangeCallback([&](Ref<Scene>& scene) { mPanelManager.GetPanel<SceneHierarchyPanel>()->SetSceneContext(scene.Raw()); });
    }

    void Editor::OnUpdate()
    {
        Resize();
        mActiveProject.Update(mCamera);
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
        mActiveProject.SetState(ProjectState::Play);
        mActiveProject.OnRuntimeStart();
    }

    void Editor::OnRuntimeEnd()
    {
        mActiveProject.OnRuntimeEnd();
        mActiveProject.SetState(ProjectState::Edit);
    }

    void Editor::Resize()
    {
        ViewportPanel* viewportPanel = mPanelManager.GetPanel<ViewportPanel>();
        glm::vec2 viewportSize = viewportPanel->GetViewportSize();
        Ref<Framebuffer> framebuffer = mRenderer->GetFinalPassFramebuffer();
        Ref<Scene>& activeScene = mActiveProject.GetActiveScene();
        if (mActiveProject.GetState() == ProjectState::Play && activeScene && activeScene->GetMainCameraEntity().Data1 && activeScene->GetMainCameraEntity().Data1->GetAspectRatio() != (viewportSize.x / viewportSize.y))
            activeScene->OnResize(viewportSize.x, viewportSize.y);
        if (FramebufferSpecification spec = framebuffer->GetSpecification(); viewportSize.x > 0.0f && viewportSize.y > 0.0f && (spec.Width != viewportSize.x || spec.Height != viewportSize.y))
        {
            mRenderer->SetRenderArea((Uint)viewportSize.x, (Uint)viewportSize.y);
            mCamera.SetViewportSize(viewportSize);

            if (mActiveProject.GetState() == ProjectState::Edit)
                activeScene->OnResize(viewportSize.x, viewportSize.y);
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