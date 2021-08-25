#include <Surge/Surge.hpp>

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
        Surge::Log<Surge::LogSeverity::Info>("Initialized!");
    }

    virtual void OnUpdate() override
    {
        //Surge::Log<Surge::LogSeverity::Trace>("MousePosition: {0}, {1}", Surge::Input::GetMousePositionX(), Surge::Input::GetMousePositionY());
    }

    virtual void OnEvent(Surge::Event& e) override
    {
        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](Surge::KeyPressedEvent& e)
        {
            Surge::Log<Surge::LogSeverity::Debug>("{0}", e.ToString());
        });
        dispatcher.Dispatch<Surge::MouseMovedEvent>([this](Surge::MouseMovedEvent& e)
        {
            Surge::Log<Surge::LogSeverity::Trace>("{0}", e.ToString());
        });
        dispatcher.Dispatch<Surge::MouseButtonPressedEvent>([this](Surge::MouseButtonPressedEvent& e)
        {
            Surge::Log<Surge::LogSeverity::Debug>("{0}", e.ToString());
        });
        dispatcher.Dispatch<Surge::WindowResizeEvent>([this](Surge::WindowResizeEvent& e)
        {
            Surge::Log<Surge::LogSeverity::Info>("{0}", e.ToString());
        });
        dispatcher.Dispatch<Surge::WindowClosedEvent>([this](Surge::WindowClosedEvent& e)
        {
            Surge::Log<Surge::LogSeverity::Info>("{0}", e.ToString());
        });
    }

    virtual void OnShutdown() override
    {
        Surge::Log<Surge::LogSeverity::Debug>("Shutdown... RIP");
    }
};

int main()
{
    MyApp app;
    Surge::Initialize(&app);
    Surge::Run();
    Surge::Shutdown();
}
