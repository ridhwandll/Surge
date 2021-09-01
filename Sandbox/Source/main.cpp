#include <Surge/Surge.hpp>

class MyApp : public Surge::Application
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

    virtual void OnEvent(Surge::Event& e) override
    {
        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](Surge::KeyPressedEvent& e)
            {
                Surge::Log("{0}", e.ToString());
            });
    }

    virtual void OnShutdown() override
    {
    }
};

int main()
{
    MyApp app;
    Surge::Initialize(&app);
    Surge::Run();
    Surge::Shutdown();
}
