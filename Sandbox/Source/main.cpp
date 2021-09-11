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
        //Surge::GPUMemoryStats memoryStatus = Surge::GetRenderContext()->GetMemoryStatus();
        //Surge::Log<Surge::LogSeverity::Info>("Used: {0} | Free: {1}", memoryStatus.Used, memoryStatus.Free);
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
};

int main()
{
    MyApp* app = new MyApp();
    Surge::Initialize(app);
    Surge::Run();
    Surge::Shutdown();
}
