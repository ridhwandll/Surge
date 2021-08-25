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
        if (Surge::Input::IsKeyPressed(Surge::Key::X))
            Surge::Input::SetCursorMode(Surge::CursorMode::Locked);
        else
            Surge::Input::SetCursorMode(Surge::CursorMode::Normal);

        //Surge::Log<Surge::LogSeverity::Trace>("MousePosition: {0}, {1}", Surge::Input::GetMousePositionX(), Surge::Input::GetMousePositionY());
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
