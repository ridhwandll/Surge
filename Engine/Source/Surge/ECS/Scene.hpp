// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include <entt.hpp>

namespace Surge
{
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

        template <typename T>
        bool HasComponent()
        {
            return mScene->GetRegistry().any_of<T>(mEnttHandle);
        }

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
