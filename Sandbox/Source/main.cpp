#include "Core/Core.hpp"
#include "Core/Clock.hpp"
#include "Core/Logger.hpp"

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
        Surge::Log<Surge::LogSeverity::INFO>("Initialized!");
    }

    virtual void OnUpdate() override
    {
        float life = Surge::Clock::GetLife();
        Surge::Log<Surge::LogSeverity::TRACE>("Updating... Time since start {0} Seconds", life);

        // Runs the engine for 15 seconds
        if (life >= 15.0f)
            Surge::Close();
    }

    virtual void OnShutdown() override
    {
        Surge::Log<Surge::LogSeverity::INFO>("Shutdown... RIP");
    }
};

int main()
{
    MyApp app;
    SURGE_BASIC_APP(app)
}
