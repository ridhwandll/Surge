#include <Surge/Surge.hpp>

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
    }

    virtual void OnUpdate() override
    {
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
