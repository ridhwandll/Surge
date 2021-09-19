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
        CoreGetRenderer()->RenderRectangle(mColor);
    }

    virtual void OnImGuiRender() override
    {
        Surge::GPUMemoryStats memoryStatus = CoreGetRenderContext()->GetMemoryStatus();

        ImGui::Begin("General");
        ImGui::ColorEdit3("Color", glm::value_ptr(mColor));
        ImGui::TextUnformatted("GPU memory status:");
        ImGui::Text("Used: %lli | Free: %lli", memoryStatus.Used, memoryStatus.Free);
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
    glm::vec3 mColor = glm::vec3(1.0f, 0.5f, 0.0f);
};

int main()
{
    MyApp* app = new MyApp();
    Surge::Initialize(app);
    Surge::Run();
    Surge::Shutdown();
}
