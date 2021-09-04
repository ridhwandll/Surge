#include <Surge/Surge.hpp>

using namespace Surge; //Ooof

struct Vertex
{
    glm::vec2 Pos;
    glm::vec3 Color;
};

const std::vector<Vertex> vertices =
{
    {{ 0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }},
    {{ 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},
    {{-0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
    {{-0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f }}
};

const std::vector<Uint> indices =
{   
    0, 1, 2,
    1, 2, 3
};

const glm::mat4 transformMatrix = glm::mat4(1.0f);

class MyApp : public Application
{
public:
    Ref<Buffer> mVertexBuffer;
    Ref<Buffer> mIndexBuffer;
    Ref<Buffer> mUniformBuffer;
    Ref<Shader> mShader;

    virtual void OnInitialize() override
    {
        mVertexBuffer = Buffer::Create(vertices.data(), static_cast<Uint>(sizeof(vertices[0]) * vertices.size()), BufferType::VertexBuffer);
        mIndexBuffer = Buffer::Create(indices.data(), static_cast<Uint>(sizeof(indices[0]) * indices.size()), BufferType::IndexBuffer);
        mUniformBuffer = Buffer::Create(&transformMatrix, sizeof(glm::mat4), BufferType::UniformBuffer);
        mShader = Shader::Create("Engine/Assets/Shaders/Simple.glsl");
    }

    virtual void OnUpdate() override
    {
        //Surge::GPUMemoryStats memoryStatus = Surge::GetRenderContext()->GetMemoryStatus();
        //Surge::Log<Surge::LogSeverity::Info>("Used: {0} | Free: {1}", memoryStatus.Used, memoryStatus.Free);

        glm::mat4 newMatrix = glm::mat4(2.0f);
        glm::mat4 finalMatrix = transformMatrix + newMatrix;

        mUniformBuffer->SetData(&finalMatrix, sizeof(glm::mat4));
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
