#include "Core/Core.hpp"
#include "Core/Clock.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Core/ThreadPool.hpp"
#include "Core/Defines.hpp"
#include "Core/Input.hpp"

void MultithreadedFunction(const Surge::String message)
{
    // Do some stuff
    Surge::Log<Surge::LogSeverity::INFO>("{0}", message);
}

int MultithreadedFuture(int value)
{
    int foo = value * 5;
    return foo;
}

class MyApp : public Surge::Application
{
public:
    virtual void OnInitialize() override
    {
        Surge::Log<Surge::LogSeverity::INFO>("Initialized!");
        
        // Make a scoped frame pool if the function that must be multithreaded are done in a single scope
        Surge::ThreadPool pool;

        pool.PushTask(MultithreadedFunction, "Hello from thread  1");
        pool.PushTask(MultithreadedFunction, "Hello from thread  2");

        std::future<int> bar = pool.Submit(MultithreadedFuture, 3);
        Surge::Log<Surge::LogSeverity::TRACE>("MultithreadedFuture returned {0}", bar.get());

        // The thread pool automatically gets destroyed at the end of scope
        // You can also have the pool as part of the application members, but it should be used only if you have multithreaded
        // functions to execute at runtime.
    }

    virtual void OnUpdate() override
    {
        float life = Surge::Clock::GetLife();
        Surge::Log<Surge::LogSeverity::TRACE>("Updating... Time since start {0} Seconds", life);
    
        if (Surge::Input::GetKeyDown(Surge::Key::Z))
            Surge::Log<Surge::LogSeverity::INFO>("The Z key was pressed!");
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
