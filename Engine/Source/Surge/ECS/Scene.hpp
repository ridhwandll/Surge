// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Surge/Core/Memory.hpp"
#include <entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    // TODO: Move to another file
    struct TransformComponent
    {
        TransformComponent() = default;
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
            : Position(position), Rotation(rotation), Scale(scale) {}

        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        glm::mat4 GetTransform()
        {
            const glm::mat4 rot = glm::toMat4(glm::quat(Rotation));
            glm::mat4 result = glm::translate(glm::mat4(1.0f), Position) * rot * glm::scale(glm::mat4(1.0f), Scale);
            return result;
        }
    };

    class Scene;
    class Entity;

    class Scene : public RefCounted
    {
    public:
        Scene() = default;
        Scene(bool runtime);
        ~Scene();

        void CreateEntity(Entity& outEntity);
        void DestroyEntity(Entity& InEntity);

        entt::registry& GetRegistry() { return mRegistry; }

    private:
        entt::registry mRegistry;
    };

    class Entity
    {
    public:
        Entity() = default;
        Entity(const entt::entity& handle, Scene* scene)
            : mEnttHandle(handle), mScene(scene) {}

        template <typename T>
        T& GetComponent()
        {
            T& component = mScene->GetRegistry().get<T>(mEnttHandle);
            return component;
        }

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            T& component = mScene->GetRegistry().emplace<T>(mEnttHandle, std::forward<Args>(args)...);
            return component;
        }

        template <typename T>
        void RemoveComponent()
        {
            mScene->GetRegistry().remove_if_exists<T>(mEnttHandle);
        }

        // TODO
        //template <typename T>
        //bool HasComponent()
        //{
        //    return mScene->GetRegistry().has<T>(mEnttHandle);
        //}

        entt::entity Raw()
        {
            return mEnttHandle;
        }

        bool IsValid()
        {
            return mScene->GetRegistry().valid(mEnttHandle);
        }

    private:
        entt::entity mEnttHandle;
        Scene* mScene;
    };

} // namespace Surge
