#include <Surge.hpp>

void MultithreadedFunction(const Surge::String message)
{
    // Do some stuff
    Surge::Log<Surge::LogSeverity::Info>("{0}", message);
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
        Surge::Log<Surge::LogSeverity::Info>("Initialized!");
        
        // Make a scoped frame pool if the function that must be multithreaded are done in a single scope
        Surge::ThreadPool threadPool(5);

        threadPool.PushTask(MultithreadedFunction, "Hello from thread  1");
        threadPool.PushTask(MultithreadedFunction, "Hello from thread  2");

        std::future<int> bar = threadPool.Submit(MultithreadedFuture, 3);
        Surge::Log<Surge::LogSeverity::Trace>("MultithreadedFuture returned {0}", bar.get());

        // The thread pool automatically gets destroyed at the end of scope
        // You can also have the pool as part of the application members, but it should be used only if you have multithreaded
        // functions to execute at runtime.
    }

    virtual void OnUpdate() override
    {
        if (Surge::Input::IsKeyPressed(Surge::Key::X))
            Surge::Input::SetCursorMode(Surge::CursorMode::Locked);
        else
            Surge::Input::SetCursorMode(Surge::CursorMode::Normal);

        Surge::Log<Surge::LogSeverity::Trace>("MousePosition: {0}, {1}", Surge::Input::GetMousePositionX(), Surge::Input::GetMousePositionY());
    }

    virtual void OnShutdown() override
    {
        Surge::Log<Surge::LogSeverity::Debug>("Shutdown... RIP");
    }
};

int main()
{
    MyApp app;
    SURGE_BASIC_APP(app)
}
