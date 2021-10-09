#include <Surge/Surge.hpp>

using namespace Surge; //Ooof

class MyApp : public Application
{
public:
    virtual void OnInitialize() override
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

    virtual void OnUpdate() override
    {
        mCamera.OnUpdate();
        if (mViewportSize.y != 0)
            mCamera.SetViewportSize({mViewportSize.x, mViewportSize.y});

        mRenderer->BeginFrame(mCamera);
        mRenderer->SubmitMesh(mEntity.GetComponent<MeshComponent>().Mesh, mEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->SubmitMesh(mOtherEntity.GetComponent<MeshComponent>().Mesh, mOtherEntity.GetComponent<TransformComponent>().GetTransform());
        mRenderer->EndFrame();
    }

    virtual void OnImGuiRender() override
    {
        RenderContext* renderContext = SurgeCore::GetRenderContext();
        Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();

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
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        if (ImGui::Begin("Viewport"))
        {
            const Ref<Image2D>& outputImage = mRenderer->GetData()->OutputFrambuffer->GetColorAttachment(0);
            mViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
            ImGuiUtils::Image(outputImage, mViewportSize);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    virtual void OnEvent(Event& e) override
    {
        mCamera.OnEvent(e);
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](KeyPressedEvent& e) { /*Log("{0}", e.ToString());*/ });
    }

    virtual void OnShutdown() override
    {
        mScene->DestroyEntity(mEntity);
        mScene->DestroyEntity(mOtherEntity);
    }

private:
    EditorCamera mCamera;
    glm::vec2 mViewportSize;
    Renderer* mRenderer;

    Ref<Scene> mScene;
    Entity mEntity;
    Entity mOtherEntity;
};

int main()
{
    ApplicationOptions appOptions;
    appOptions.EnableImGui = true;

    MyApp* app = new MyApp();
    app->SetAppOptions(appOptions);

    SurgeCore::Initialize(app);
    SurgeCore::Run();
    SurgeCore::Shutdown();
}
