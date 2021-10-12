// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Editor.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Panels/ViewportPanel.hpp"

namespace Surge
{
    void Editor::OnInitialize()
    {
        mRenderer = SurgeCore::GetRenderer();
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mScene = Ref<Scene>::Create(false);

        mScene->CreateEntity(mEntity);
        mEntity.AddComponent<TransformComponent>();
        mEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create("Engine/Assets/Mesh/Vulkan.obj"));

        mScene->CreateEntity(mOtherEntity);
        mOtherEntity.AddComponent<TransformComponent>(glm::vec3(15.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
        mOtherEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create("Engine/Assets/Mesh/Vulkan.obj"));

        mCamera.SetActive(true);

        mPanelManager = PanelManager();
        mPanelManager.PushPanel<ViewportPanel>(nullptr);
        { // TransformComponent reflection test
            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<TransformComponent>();
            Log<Severity::Info>("Class: {0}", clazz->GetName());
            Log<Severity::Info>(" Variable(s):");

            for (const auto& [name, variable] : clazz->GetVariables())
            {
                Log<Severity::Info>("  {0} | {2} | {1}", name, variable.GetSize(), variable.GetType().GetFullName());
            }
            Log<Severity::Info>(" Function(s):");
            for (const auto& [name, func] : clazz->GetFunctions())
            {
                Log<Severity::Info>("  {0} | ReturnType: {1}", name, func.GetReturnType().GetFullName());
            }
        }
    }

    void Editor::OnUpdate()
    {
        mCamera.OnUpdate();

        ViewportPanel* vp = mPanelManager.GetPanel<ViewportPanel>();
        if (vp->GetViewportSize().y > 0)
        {
            glm::vec2 viewportSize = vp->GetViewportSize();
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
        RenderContext* renderContext = SurgeCore::GetRenderContext();
        Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();
        mTitleBar.Render();

        ImGuiAux::BeginDockSpace();
        if (ImGui::Begin("Settings and Stats"))
        {
            TransformComponent& transformComponent = mEntity.GetComponent<TransformComponent>();

            ImGui::DragFloat3("Translation", glm::value_ptr(transformComponent.Position), 0.1);
            ImGui::DragFloat3("Rotation", glm::value_ptr(transformComponent.Rotation), 0.1);
            ImGui::DragFloat3("Scale", glm::value_ptr(transformComponent.Scale), 0.1);
            ImGui::TextUnformatted("Status:");
            float used = memoryStatus.Used / 1000000.0f;
            float free = memoryStatus.Free / 1000000.0f;
            ImGui::Text("Device: %s", SurgeCore::GetRenderContext()->GetGPUInfo().Name.c_str());
            ImGui::Text("Used: %f Mb | Local-Free: %f Mb | Total: %f Mb", used, free, used + free);

            if (ImGui::TreeNode("Shaders"))
            {
                Vector<Ref<Shader>>& allAhaders = mRenderer->GetData()->ShaderSet.GetAllShaders();
                for (Ref<Shader>& shader : allAhaders)
                {
                    if (ImGui::TreeNode(Filesystem::GetNameWithoutExtension(shader->GetPath()).c_str()))
                    {
                        ImGui::PushID(shader->GetPath().c_str());
                        if (ImGui::Button("Reload"))
                            shader->Reload();

                        ImGui::PopID();
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            ImGui::Text("Frame Time: % .2f ms ", Clock::GetMilliseconds());
            ImGui::Text("FPS: % .2f", ImGui::GetIO().Framerate);
        }
        ImGui::End();

        // Viewport
        mPanelManager.RenderAll();

        ImGuiAux::EndDockSpace();
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
