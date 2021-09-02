#include <Surge/Surge.hpp>

using namespace Surge; //Ooof

struct Vertex
{
    glm::vec2 Pos;
    glm::vec3 Color;
};

const std::vector<Vertex> vertices =
{
    {{ 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
    {{ 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
    {{-0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }}
};

class MyApp : public Application
{
public:
    Ref<Buffer> buffer;
    virtual void OnInitialize() override
    {
        buffer = Buffer::Create(vertices.data(), static_cast<Uint>(sizeof(vertices[0]) * vertices.size()), BufferType::VertexBuffer);
    }

    virtual void OnUpdate() override
    {
        //Surge::GPUMemoryStats memoryStatus = Surge::GetRenderContext()->GetMemoryStatus();
        //Surge::Log<Surge::LogSeverity::Info>("Used: {0} | Free: {1}", memoryStatus.Used, memoryStatus.Free);
    }

    virtual void OnEvent(Event& e) override
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::KeyPressedEvent>([this](KeyPressedEvent& e)
            {
                Log("{0}", e.ToString());
            });
    }

    virtual void OnShutdown() override
    {
    }
};

int main()
{
    MyApp* app = new MyApp();
    Surge::Initialize(app);
    Surge::Run();
    Surge::Shutdown();
}
