#include "Core/Core.hpp"
#include "Core/Clock.hpp"
#include <iostream>

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
        std::cout << "Initialized!" << std::endl;
    }

    virtual void OnUpdate() override
    {
        float life = Surge::Clock::GetLife();
        std::cout << "Updating... Time since start " << life << " Seconds" << std::endl;

        // Runs the engine for 15 seconds
        if (life >= 15.0f)
            Surge::Close();
    }

    virtual void OnShutdown() override
    {
        std::cout << "Shutdown... RIP" << std::endl;
    }
};

int main()
{
    MyApp app;

    Surge::Initialize(&app);
    Surge::Run();
    Surge::Shutdown();
}