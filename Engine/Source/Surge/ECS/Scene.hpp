// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Memory.hpp"
#include "Surge/Core/UUID.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Camera/RuntimeCamera.hpp"
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

        void OnRuntimeStart();
        void Update();
        void Update(const EditorCamera& camera);
        void OnRuntimeEnd();
        void CopyTo(Scene* other);
        void CreateEntity(Entity& outEntity, const String& name = "New Entity");
        void CreateEntityWithID(Entity& outEntity, const UUID& id, const String& name = "New Entity");
        void DestroyEntity(Entity& entity);
        void OnResize(Uint width, Uint height);

        entt::registry& GetRegistry() { return mRegistry; }
        Pair<RuntimeCamera*, glm::mat4> GetMainCameraEntity(); // Camera - CameraTransform(view = glm::inverse(CameraTransform))

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
        bool HasComponent() const
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

        Scene* GetScene() const
        {
            return mScene;
        }

        operator bool() const { return mEnttHandle != entt::null; }
        operator entt::entity() const { return mEnttHandle; }
        bool operator==(const Entity& other) const { return mEnttHandle == other.mEnttHandle && mScene == other.mScene; }
        bool operator!=(const Entity& other) const { return !(*this == other); }

    private:
        entt::entity mEnttHandle = entt::null;
        Scene* mScene = nullptr;
    };

} // namespace Surge
