#include <Surge/Surge.hpp>
#include "Surge/Graphics/Abstraction/Vulkan/VulkanTexture.hpp"
#include "Backends/imgui_impl_vulkan.h"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"

using namespace Surge; //Ooof

class MyApp : public Application
{
public:
    virtual void OnInitialize() override
    {
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mMesh = Ref<Mesh>::Create("Engine/Assets/Mesh/Cube.fbx");
        mCamera.SetActive(true);
    }

    virtual void OnUpdate() override
    {
        const glm::mat4 rot = glm::toMat4(glm::quat(mRotation));
        mTransform = glm::translate(glm::mat4(1.0f), mPosition) * rot * glm::scale(glm::mat4(1.0f), mScale);
        mOtherTransform = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f)) * rot * glm::scale(glm::mat4(1.0f), mScale);

        mCamera.OnUpdate();
        mCamera.SetViewportSize(CoreGetWindow()->GetSize());

        CoreGetRenderer()->BeginFrame(mCamera);
        CoreGetRenderer()->SubmitMesh(mMesh, mTransform);
        CoreGetRenderer()->SubmitMesh(mMesh, mOtherTransform);
        CoreGetRenderer()->EndFrame();
    }

    virtual void OnImGuiRender() override
    {
        Surge::GPUMemoryStats memoryStatus = CoreGetRenderContext()->GetMemoryStatus();

        if (ImGui::Begin("Settings"))
        {
            ImGui::DragFloat3("Translation", glm::value_ptr(mPosition), 0.1);
            ImGui::DragFloat3("Rotation", glm::value_ptr(mRotation), 0.1);
            ImGui::DragFloat3("Scale", glm::value_ptr(mScale), 0.1);
            ImGui::TextUnformatted("Status:");
            float used = memoryStatus.Used / 1000000.0f;
            float free = memoryStatus.Free / 1000000.0f;
            ImGui::Text("Device: %s", CoreGetRenderContext()->GetGPUInfo().Name.c_str());
            ImGui::Text("Used: %f Mb | Local-Free: %f Mb | Total: %f Mb", used, free, used + free);
        }
        ImGui::End();
    }

    virtual void OnEvent(Event& e) override
    {
        mCamera.OnEvent(e);
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](KeyPressedEvent& e) {
            //Log("{0}", e.ToString());
        });
    }

    virtual void OnShutdown() override
    {
    }

private:
    glm::vec3 mPosition = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 mRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 mTransform;
    glm::mat4 mOtherTransform;

    Ref<Mesh> mMesh;
    EditorCamera mCamera;
};

int main()
{
    MyApp* app = new MyApp();
    Surge::Initialize(app);
    Surge::Run();
    Surge::Shutdown();
}
