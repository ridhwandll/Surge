#include "Core/Core.hpp"
#include "Core/Clock.hpp"
#include "Core/Logger.hpp"
#include "Core/Window.hpp"
#include "Core/ThreadPool.hpp"
#include "Core/Defines.hpp"

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

        mPool.PushTask(MultithreadedFunction, "Hello from thread  1");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  2");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  3");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  4");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  5");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  6");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  7");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  8");
        mPool.PushTask(MultithreadedFunction, "Hello from thread  9");
        mPool.PushTask(MultithreadedFunction, "Hello from thread 10");
        mPool.PushTask(MultithreadedFunction, "Hello from thread 11");
        mPool.PushTask(MultithreadedFunction, "Hello from thread 12");

        std::future<int> bar = mPool.Submit(MultithreadedFuture, 3);
        Surge::Log<Surge::LogSeverity::TRACE>("MultithreadedFuture returned {0}", bar.get());

        mPool.WaitForTasks();
        mPool.Reset();
    }

    virtual void OnUpdate() override
    {
        float life = Surge::Clock::GetLife();
        Surge::Log<Surge::LogSeverity::TRACE>("Updating... Time since start {0} Seconds", life);
    }

    virtual void OnShutdown() override
    {
        Surge::Log<Surge::LogSeverity::INFO>("Shutdown... RIP");
    }
private:
    Surge::ThreadPool mPool;
};

int main()
{
    MyApp app;
    SURGE_BASIC_APP(app)
}
