#include <Surge/Surge.hpp>

using namespace Surge; //Ooof

class MyApp : public Application
{
public:
    virtual void OnInitialize() override
    {
    }

    virtual void OnUpdate() override
    {
        CoreGetRenderer()->RenderRectangle(mPosition);
    }

    virtual void OnImGuiRender() override
    {
        Surge::GPUMemoryStats memoryStatus = CoreGetRenderContext()->GetMemoryStatus();

        if (ImGui::Begin("Settings"))
        {
            ImGui::DragFloat3("Translation", glm::value_ptr(mPosition), 0.1);
            ImGui::TextUnformatted("GPU memory status:");
            ImGui::Text("Used: %lli | Free: %lli Bytes", memoryStatus.Used, memoryStatus.Free);
        }
        ImGui::End();
    }

    virtual void OnEvent(Event& e) override
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](KeyPressedEvent& e)
            {
                Log("{0}", e.ToString());
            });
    }

    virtual void OnShutdown() override
    {
    }
private:
    glm::vec3 mPosition = glm::vec3(0.0f, 0.0f, 1.0f);
};

int main()
{
    MyApp* app = new MyApp();
    Surge::Initialize(app);
    Surge::Run();
    Surge::Shutdown();
}
