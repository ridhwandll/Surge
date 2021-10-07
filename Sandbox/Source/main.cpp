#include <Surge/Surge.hpp>
#include <SurgeReflect/SurgeReflect.hpp>

using namespace Surge; //Ooof

struct ReflectableStruct
{
    uint64_t X;
    float GetAFloat() { return 3.1416f; }
    SURGE_REFLECTION_ENABLE;
};

class MyApp : public Application
{
public:
    virtual void OnInitialize() override
    {
        mCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        mMesh = Ref<Mesh>::Create("Engine/Assets/Mesh/Vulkan.obj");
        mCamera.SetActive(true);
        mRenderer = SurgeCore::GetRenderer();

        // Testing the new reflection system
        const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<ReflectableStruct>();
        Log<Severity::Info>("Class: {0}", clazz->GetName());
        Log<Severity::Info>("  Variables:-");

        for (const auto& [name, variable] : clazz->GetVariables())
        {
            Log<Severity::Info>("    {0} | {2} | {1}", name, variable.GetSize(), variable.GetType().GetFullName());
        }

        Log<Severity::Info>("  Functions:-");
        for (const auto& [name, func] : clazz->GetFunctions())
        {
            Log<Severity::Info>("    {0} | ReturnType: {1}", name, func.GetReturnType().GetFullName());
        }
    }

    virtual void OnUpdate() override
    {
        const glm::mat4 rot = glm::toMat4(glm::quat(mRotation));
        mTransform = glm::translate(glm::mat4(1.0f), mPosition) * rot * glm::scale(glm::mat4(1.0f), mScale);

        mCamera.OnUpdate();
        if (mViewportSize.y != 0)
            mCamera.SetViewportSize({mViewportSize.x, mViewportSize.y});

        mRenderer->BeginFrame(mCamera);
        mRenderer->SubmitMesh(mMesh, mTransform);
        mRenderer->EndFrame();
    }

    virtual void OnImGuiRender() override
    {
        RenderContext* renderContext = SurgeCore::GetRenderContext();
        Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();

        if (ImGui::Begin("Settings"))
        {
            ImGui::DragFloat3("Translation", glm::value_ptr(mPosition), 0.1);
            ImGui::DragFloat3("Rotation", glm::value_ptr(mRotation), 0.1);
            ImGui::DragFloat3("Scale", glm::value_ptr(mScale), 0.1);
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

    Ref<Mesh> mMesh;
    EditorCamera mCamera;
    glm::vec2 mViewportSize;
    Renderer* mRenderer;
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

// clang-format off
SURGE_REFLECT_CLASS_REGISTER_BEGIN(ReflectableStruct)
    .AddVariable<&ReflectableStruct::X>("X", SurgeReflect::AccessModifier::Public)
    .AddFunction<&ReflectableStruct::GetAFloat>("GetAFloat", SurgeReflect::AccessModifier::Public)
SURGE_REFLECT_CLASS_REGISTER_END(ReflectableStruct)
