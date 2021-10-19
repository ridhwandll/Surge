// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Serializer/Serializer.hpp"
#include "Surge/ECS/Components.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <json/json.hpp>

// https://github.com/nlohmann/json#arbitrary-types-conversions
namespace glm
{
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

} // namespace glm

namespace Surge
{
    // Enum Mapping(s)
    NLOHMANN_JSON_SERIALIZE_ENUM(RuntimeCamera::ProjectionType, {{RuntimeCamera::ProjectionType::Perspective, "Perspective"}, {RuntimeCamera::ProjectionType::Orthographic, "Orthographic"}})

    template <typename XComponent>
    static void SerializeComponent(nlohmann::json& j, Entity e)
    {
        if (e.HasComponent<XComponent>())
        {
            XComponent comp = e.GetComponent<XComponent>();
            const SurgeReflect::Class* clazz = SurgeReflect::GetReflection<XComponent>();
            Uint offset = 0;

            nlohmann::json& out = j[clazz->GetName()];

            for (const auto& [name, var] : clazz->GetVariables())
            {
                uint64_t size = var.GetSize();
                const SurgeReflect::Type& type = var.GetType();
                if (type.EqualTo<bool>())
                {
                    byte* source = reinterpret_cast<byte*>(&comp) + offset;
                    bool destination;
                    std::memcpy(&destination, reinterpret_cast<bool*>(source), size);
                    out[name] = destination;
                }
                else if (type.EqualTo<String>())
                {
                    byte* source = reinterpret_cast<byte*>(&comp) + offset;

                    // Get the string size; (we cannot use var.GetSize() as that uses sizeof())
                    String* src = (String*)source;
                    size = src->size();

                    String destination;
                    destination.resize(size);
                    std::memcpy(reinterpret_cast<void*>(destination.data()), reinterpret_cast<String*>(source)->c_str(), size);
                    out[name] = destination;
                }
                else if (type.EqualTo<glm::vec3>())
                {
                    byte* source = reinterpret_cast<byte*>(&comp) + offset;
                    glm::vec3 destination;
                    std::memcpy(glm::value_ptr(destination), glm::value_ptr(*reinterpret_cast<glm::vec3*>(source)), size);
                    out[name] = destination;
                }
                else if (type.EqualTo<RuntimeCamera>())
                {
                    byte* source = reinterpret_cast<byte*>(&comp) + offset;
                    RuntimeCamera* cam = reinterpret_cast<RuntimeCamera*>(source);

                    out["Vertical FOV"] = cam->GetPerspectiveVerticalFOV();
                    out["Perspective NearClip"] = cam->GetPerspectiveNearClip();
                    out["Perspective FarClip"] = cam->GetPerspectiveFarClip();
                    out["Orthographic NearClip"] = cam->GetOrthographicNearClip();
                    out["Orthographic FarClip"] = cam->GetOrthographicFarClip();
                    out["Orthographic Size"] = cam->GetOrthographicSize();
                    out["Projection"] = cam->GetProjectionType();
                    
                }
                else if (type.EqualTo<Ref<Mesh>>())
                {
                    byte* source = reinterpret_cast<byte*>(&comp) + offset;
                    Ref<Mesh>* mesh = reinterpret_cast<Ref<Mesh>*>(source);
                    String path = (*mesh)->GetPath();
                    out["Path"] = path;
                }

                offset += size;
            }
        }
    }

    static void SerializeEntity(nlohmann::json& j, Entity& e)
    {
        nlohmann::json& out = j[std::to_string(e.GetComponent<IDComponent>().ID.Get())];
        SerializeComponent<NameComponent>(out, e);
        SerializeComponent<TransformComponent>(out, e);
        SerializeComponent<CameraComponent>(out, e);
        SerializeComponent<MeshComponent>(out, e);
    }

    template <>
    void Surge::Serializer::Serialize(const Path& path, Scene* out)
    {
        SCOPED_TIMER("Serialization");
        nlohmann::json outJson = nlohmann::json();

        out->GetRegistry().each([&](auto entityID) {
            Entity e = Entity(entityID, out);
            SerializeEntity(outJson["Scene"], e);
        });

        String result = outJson.dump(4);
        FILE* f;
        errno_t e = fopen_s(&f, path.c_str(), "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

} // namespace Surge
