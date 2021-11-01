// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Serializer/Serializer.hpp"
#include "Surge/ECS/Components.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <json/json.hpp>
#include <filesystem>
#include <fstream>

// https://github.com/nlohmann/json#arbitrary-types-conversions
namespace glm
{
    inline void to_json(nlohmann::json& j, const vec2& p)
    {
        j = nlohmann::json {{"X", p.x}, {"Y", p.y}};
    }

    inline void from_json(const nlohmann::json& j, vec2& p)
    {
        j.at("X").get_to(p.x);
        j.at("Y").get_to(p.y);
    }

    inline void to_json(nlohmann::json& j, const vec3& p)
    {
        j = nlohmann::json {{"X", p.x}, {"Y", p.y}, {"Z", p.z}};
    }

    inline void from_json(const nlohmann::json& j, vec3& p)
    {
        j.at("X").get_to(p.x);
        j.at("Y").get_to(p.y);
        j.at("Z").get_to(p.z);
    }

    inline void to_json(nlohmann::json& j, const vec4& p)
    {
        j = nlohmann::json {{"X", p.x}, {"Y", p.y}, {"Z", p.z}, {"W", p.w}};
    }

    inline void from_json(const nlohmann::json& j, vec4& p)
    {
        j.at("X").get_to(p.x);
        j.at("Y").get_to(p.y);
        j.at("Z").get_to(p.z);
        j.at("W").get_to(p.w);
    }
} // namespace glm

namespace Surge
{
    //----------
    // Serialize
    //----------

    // Enum Mapping(s)
    NLOHMANN_JSON_SERIALIZE_ENUM(RuntimeCamera::ProjectionType, {{RuntimeCamera::ProjectionType::Perspective, "Perspective"}, {RuntimeCamera::ProjectionType::Orthographic, "Orthographic"}})

    template <typename XComponent>
    FORCEINLINE static void SerializeComponent(nlohmann::json& j, Entity& e)
    {
        if (e.HasComponent<XComponent>())
        {
            XComponent& comp = e.GetComponent<XComponent>();
            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<XComponent>();
            uint64_t offset = 0;

            nlohmann::json& out = j[clazz->GetName()];

            for (const auto& [name, var] : clazz->GetVariables())
            {
                uint64_t size = var.GetSize();
                const byte* source = reinterpret_cast<const byte*>(&comp) + offset;

                const SurgeReflect::Type& type = var.GetType();
                if (type.EqualTo<bool>())
                {
                    bool destination;
                    std::memcpy(&destination, reinterpret_cast<const bool*>(source), size);
                    out[name] = destination;
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<float>())
                {
                    float destination;
                    std::memcpy(&destination, reinterpret_cast<const float*>(source), size);
                    out[name] = destination;
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<UUID>())
                {
                    uint64_t destination;
                    std::memcpy(&destination, reinterpret_cast<const uint64_t*>(source), size);
                    out[name] = destination;
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<String>())
                {
                    // Get the string size; (we cannot use var.GetSize() as that uses sizeof())
                    const String* src = reinterpret_cast<const String*>(source);
                    size = src->size();

                    String destination;
                    destination.resize(size);
                    std::memcpy(reinterpret_cast<void*>(destination.data()), reinterpret_cast<const String*>(source)->c_str(), size);
                    out[name] = destination;
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<glm::vec3>())
                {
                    glm::vec3 destination;
                    std::memcpy(glm::value_ptr(destination), glm::value_ptr(*reinterpret_cast<const glm::vec3*>(source)), size);
                    out[name] = destination;
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<RuntimeCamera>())
                {
                    const RuntimeCamera* cam = reinterpret_cast<const RuntimeCamera*>(source);

                    out["Vertical FOV"] = cam->GetPerspectiveVerticalFOV();
                    out["Perspective NearClip"] = cam->GetPerspectiveNearClip();
                    out["Perspective FarClip"] = cam->GetPerspectiveFarClip();
                    out["Orthographic NearClip"] = cam->GetOrthographicNearClip();
                    out["Orthographic FarClip"] = cam->GetOrthographicFarClip();
                    out["Orthographic Size"] = cam->GetOrthographicSize();
                    out["Projection"] = cam->GetProjectionType();
                    offset += size;
                    continue;
                }
                else if (type.EqualTo<Ref<Mesh>>())
                {
                    const Ref<Mesh>* mesh = reinterpret_cast<const Ref<Mesh>*>(source);
                    String path;
                    if (*mesh)
                        path = std::filesystem::relative((*mesh)->GetPath()).string();

                    out[name] = path;
                    offset += size;
                    continue;
                }
                SG_ASSERT_INTERNAL("Unhandled Variable Type!");
            }
        }
    }

    // nlohmann::json& j is in "Scene" scope
    static void SerializeEntity(nlohmann::json& j, Entity& e, uint64_t index)
    {
        SG_ASSERT_NOMSG(e);

        nlohmann::json& out = j[fmt::format("Entity{0}", index)];
        SerializeComponent<IDComponent>(out, e);
        SerializeComponent<NameComponent>(out, e);
        SerializeComponent<TransformComponent>(out, e);
        SerializeComponent<CameraComponent>(out, e);
        SerializeComponent<MeshComponent>(out, e);
        SerializeComponent<PointLightComponent>(out, e);
    }

    template <>
    void Surge::Serializer::Serialize(const Path& path, Ref<Scene>& in)
    {
        SG_ASSERT_NOMSG(in);
        SCOPED_TIMER("Serialization");
        nlohmann::json outJson = nlohmann::json();

        uint64_t index = 0;
        in->GetRegistry().each([&](auto entityID) {
            Entity e = Entity(entityID, in.Raw());
            SerializeEntity(outJson["Scene"], e, index);
            index++;
        });

        const size_t size = in->GetRegistry().size<IDComponent>();
        outJson["Scene"]["Size"] = size;

        String result = outJson.dump(4);
        FILE* f;
        errno_t e = fopen_s(&f, path.c_str(), "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

    //------------
    // Deserialize
    //------------

    template <typename XComponent>
    FORCEINLINE static void DeserializeComponent(nlohmann::json& j, Entity& e)
    {
        const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<XComponent>();

        // Check if the json contains the component name, if it does, then proceed with Deserialization
        bool hasComponent = j.contains(clazz->GetName());
        if (!hasComponent)
            return;

        if (!e.HasComponent<XComponent>())
            e.AddComponent<XComponent>();

        nlohmann::json& inJson = j[clazz->GetName()];
        XComponent& comp = e.GetComponent<XComponent>();
        uint64_t offset = 0;

        for (const auto& [name, var] : clazz->GetVariables())
        {
            const SurgeReflect::Type& type = var.GetType();
            uint64_t size = var.GetSize();
            byte* destination = reinterpret_cast<byte*>(&comp) + offset;

            if (type.EqualTo<bool>())
            {
                const bool source = inJson[name];
                bool* dst = reinterpret_cast<bool*>(destination);
                std::memcpy(dst, &source, size);
            }
            else if (type.EqualTo<float>())
            {
                const float source = inJson[name];
                float* dst = reinterpret_cast<float*>(destination);
                std::memcpy(dst, &source, size);
            }
            else if (type.EqualTo<UUID>())
            {
                const uint64_t source = inJson[name];
                uint64_t* dst = reinterpret_cast<uint64_t*>(destination);
                std::memcpy(dst, &source, size);
            }
            else if (type.EqualTo<String>())
            {
                const String source = inJson[name];
                size = source.size(); // We cannot use var.GetSize() as that uses sizeof()

                String* src = reinterpret_cast<String*>(destination);
                src->resize(size);
                std::memcpy(src->data(), source.c_str(), size);
            }
            else if (type.EqualTo<glm::vec3>())
            {
                glm::vec3 source = inJson[name];
                glm::vec3* dst = reinterpret_cast<glm::vec3*>(destination);
                std::memcpy(dst, glm::value_ptr(source), size);
            }
            else if (type.EqualTo<RuntimeCamera>())
            {
                RuntimeCamera cam;
                cam.SetPerspectiveVerticalFOV(inJson["Vertical FOV"]);
                cam.SetPerspectiveNearClip(inJson["Perspective NearClip"]);
                cam.SetPerspectiveFarClip(inJson["Perspective FarClip"]);
                cam.SetOrthographicNearClip(inJson["Orthographic NearClip"]);
                cam.SetOrthographicFarClip(inJson["Orthographic FarClip"]);
                cam.SetOrthographicSize(inJson["Orthographic Size"]);
                cam.SetProjectionType(inJson["Projection"]);
                std::memcpy(destination, &cam, size);
            }
            else if (type.EqualTo<Ref<Mesh>>())
            {
                String path = inJson[name];

                Ref<Mesh>& src = *reinterpret_cast<Ref<Mesh>*>(destination);
                path.empty() ? src = nullptr : src = Ref<Mesh>::Create(path);
            }
            else
            {
                SG_ASSERT_INTERNAL("Unhandled Variable Type!");
            }

            offset += size;
        }
    }

    static void DeserializeEntity(nlohmann::json& j, Entity& e, uint64_t index)
    {
        SG_ASSERT_NOMSG(e);

        nlohmann::json& inJson = j[fmt::format("Entity{0}", index)];
        DeserializeComponent<IDComponent>(inJson, e);
        DeserializeComponent<NameComponent>(inJson, e);
        DeserializeComponent<TransformComponent>(inJson, e);
        DeserializeComponent<MeshComponent>(inJson, e);
        DeserializeComponent<CameraComponent>(inJson, e);
        DeserializeComponent<PointLightComponent>(inJson, e);
    }

    template <>
    void Serializer::Deserialize(const Path& path, Ref<Scene>& out)
    {
        SG_ASSERT_NOMSG(out);
        out->GetRegistry().clear();

        std::ifstream ifs(path);
        String jsonContents((std::istreambuf_iterator<char>(ifs)),
                            (std::istreambuf_iterator<char>()));

        // Parse the json
        nlohmann::json parsedJson = nlohmann::json::parse(jsonContents);
        uint64_t size = parsedJson["Scene"]["Size"];

        for (uint64_t i = 0; i < size; i++)
        {
            Entity newEntity;
            out->CreateEntity(newEntity, "");
            DeserializeEntity(parsedJson["Scene"], newEntity, i);
        }
    }
} // namespace Surge