// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Editor.hpp"
#include "Utility/ImGuiAux.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <windows.h>

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
        if (mViewportSize.y != 0)
            mCamera.SetViewportSize({mViewportSize.x, mViewportSize.y});

        mRenderer->BeginFrame(mCamera);
        mRenderer->SubmitMesh(mEntity.GetComponent<MeshComponent>().Mesh, mEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->SubmitMesh(mOtherEntity.GetComponent<MeshComponent>().Mesh, mOtherEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->EndFrame();
    }

    void Editor::OnImGuiRender()
    {
        RenderContext* renderContext = SurgeCore::GetRenderContext();
        Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

        if (ImGui::BeginMainMenuBar())
        {
            // System buttons
            {
                Window* window = SurgeCore::GetWindow();
                const float buttonSize = ImGui::GetFrameHeight();
                const float iconMargin = buttonSize * 0.33f;
                ImRect buttonRect = ImRect(ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowWidth() - buttonSize, 0.0f), ImGui::GetWindowPos() + ImGui::GetWindowSize());
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                // Exit button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("EXIT"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
                    }

                    // Render the cross
                    {
                        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
                        drawList->AddLine(buttonRect.Min + ImVec2(iconMargin, iconMargin), buttonRect.Max - ImVec2(iconMargin, iconMargin), color, 2.0f);
                        drawList->AddLine(buttonRect.Min + ImVec2(buttonRect.GetWidth() - iconMargin, iconMargin), buttonRect.Max - ImVec2(buttonRect.GetWidth() - iconMargin, iconMargin), color, 2.0f);
                    }

                    if (pressed)
                        SurgeCore::Close();

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }

                // Maximize button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("MAXIMIZE"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
                    }

                    // Render the box
                    {
                        const ImU32 color = ImGui::GetColorU32({1.0f, 1.0f, 1.0f, 1.0f});
                        drawList->AddRect(buttonRect.Min + ImVec2(iconMargin, iconMargin), buttonRect.Max - ImVec2(iconMargin, iconMargin), color, 1.0f, 0, 2.0f);
                    }

                    if (pressed)
                    {
                        if (!window->IsWindowMaximized())
                            window->Maximize();
                        else
                            window->RestoreFromMaximize();
                    }

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }

                // Minimize button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("MINIMIZE"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
                    }

                    // Render the Line
                    {
                        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
                        drawList->AddLine(buttonRect.Min + ImVec2(iconMargin, buttonRect.GetHeight() / 2), buttonRect.Max - ImVec2(iconMargin, buttonRect.GetHeight() / 2), color, 2.0f);
                    }

                    if (pressed)
                        window->Minimize();

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::PopStyleVar();

        if (ImGui::Begin("Settings"))
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
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        if (ImGui::Begin("Viewport"))
        {
            const Ref<Image2D>& outputImage = mRenderer->GetData()->OutputFrambuffer->GetColorAttachment(0);
            mViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
            ImGuiAux::Image(outputImage, mViewportSize);
        }
        ImGui::End();
        ImGui::PopStyleVar();
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
