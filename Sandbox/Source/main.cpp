#include "Core/Core.hpp"
#include "Core/Clock.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
        Surge::Log<Surge::LogSeverity::INFO>("Initialized!");

        mWindow = Surge::Window::Create(1280, 720, "Surge Window");
        Surge::Log<Surge::LogSeverity::INFO>("Create {0} ({1}, {2})", mWindow->GetTitle(), mWindow->GetWidth(), mWindow->GetHeight());
    }

    virtual void OnUpdate() override
    {
        while (mWindow->IsOpen())
        {
            float life = Surge::Clock::GetLife();
            Surge::Log<Surge::LogSeverity::TRACE>("Updating... Time since start {0} Seconds", life);

            mWindow->Update();
        }

        // When the window is closed (after the while loop), close Surge
        Surge::Close();
    }

    virtual void OnShutdown() override
    {
        delete mWindow;
        Surge::Log<Surge::LogSeverity::INFO>("Shutdown... RIP");
    }

private:
    Surge::Window* mWindow = nullptr;
};

int main()
{
    MyApp app;
    SURGE_BASIC_APP(app)
}
