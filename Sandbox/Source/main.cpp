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
            Surge::Scope<Surge::Window>& window = Surge::GetWindow();
            if (e.GetKeyCode() == Surge::Key::M)
                window->Maximize();
            else if (e.GetKeyCode() == Surge::Key::S)
                window->Minimize();
            else if (e.GetKeyCode() == Surge::Key::A)
            {
                window->SetPos({ 0, 0 });
                Surge::Log<Surge::LogSeverity::Info>("Current window pos: {0}, {1}", window->GetPos().Data1, window->GetPos().Data2);
            }
            window->SetTitle(e.ToString());
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
