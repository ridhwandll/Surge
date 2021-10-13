// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Editor.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Panels/ViewportPanel.hpp"
#include "Panels/PerformancePanel.hpp"
#include <imgui_internal.h>

namespace Surge
{
    void Editor::OnInitialize()
    {
        mRenderer = SurgeCore::GetRenderer();
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mCamera.SetActive(true);

        mScene = Ref<Scene>::Create(false);

        mScene->CreateEntity(mEntity);
        mEntity.AddComponent<TransformComponent>();
        mEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create("Engine/Assets/Mesh/Vulkan.obj"));

        mScene->CreateEntity(mOtherEntity);
        mOtherEntity.AddComponent<TransformComponent>(glm::vec3(15.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
        mOtherEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create("Engine/Assets/Mesh/Vulkan.obj"));

        mPanelManager.PushPanel<ViewportPanel>();
        mPanelManager.PushPanel<PerformancePanel>();
    }

    void Editor::OnUpdate()
    {
        mCamera.OnUpdate();

        ViewportPanel* viewportPanel = mPanelManager.GetPanel<ViewportPanel>();
        if (viewportPanel->GetViewportSize().y > 0)
        {
            glm::vec2 viewportSize = viewportPanel->GetViewportSize();
            Ref<Framebuffer> frameBuffer = mRenderer->GetData()->OutputFrambuffer;
            FramebufferSpecification spec = frameBuffer->GetSpecification();
            mCamera.SetViewportSize({viewportSize.x, viewportSize.y});
            if (spec.Width != viewportSize.x || spec.Height != viewportSize.y)
                frameBuffer->Resize(viewportSize.x, viewportSize.y);
        }

        mRenderer->BeginFrame(mCamera);
        mRenderer->SubmitMesh(mEntity.GetComponent<MeshComponent>().Mesh, mEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->SubmitMesh(mOtherEntity.GetComponent<MeshComponent>().Mesh, mOtherEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->EndFrame();
    }

    void Editor::OnImGuiRender()
    {
        mTitleBar.Render();
        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_NoWindowMenuButton);
        mPanelManager.RenderAll();
    }

    void Editor::OnEvent(Event& e)
    {
        mCamera.OnEvent(e);
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](KeyPressedEvent& e) { /*Log("{0}", e.ToString());*/ });
    }

    void Editor::OnShutdown()
    {
    }

} // namespace Surge

// Entry point
int main()
{
    Surge::ApplicationOptions appOptions;
    appOptions.EnableImGui = true;

    Surge::Editor* app = new Surge::Editor();
    app->SetAppOptions(appOptions);

    Surge::SurgeCore::Initialize(app);
    Surge::SurgeCore::Run();
    Surge::SurgeCore::Shutdown();
}
