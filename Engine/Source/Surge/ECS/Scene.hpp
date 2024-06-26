// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Memory.hpp"
#include "Surge/Core/UUID.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Camera/RuntimeCamera.hpp"
#include <entt.hpp>
#include "Components.hpp"

namespace Surge
{
    struct SURGE_API SceneMetadata
    {
        String Name;
        Path ScenePath;
        UUID SceneUUID;
    };

    class Scene;
    class Entity;
    class Project;

    class SURGE_API Scene : public RefCounted
    {
    public:
        Scene() = default;
        Scene(Project* parentProject, const SceneMetadata& sceneMetadata, bool runtime);
        Scene(Project* parentProject, const String& name, const Path& path, bool runtime);
        ~Scene();

        void OnRuntimeStart();
        void Update();                     // Runtime Update
        void Update(EditorCamera& camera); // EditorCam Update
        void OnRuntimeEnd();
        void CopyTo(Scene* other);
        Entity FindEntityByUUID(UUID id);
        SceneMetadata& GetMetadata() { return mMetadata; }
        Project* GetParentProject() { return mParentProject; }

        // Entity manipulation
        void CreateEntity(Entity& outEntity, const String& name = "New Entity");
        void CreateEntityWithID(Entity& outEntity, const UUID& id, const String& name = "New Entity");
        void ParentEntity(Entity& entity, Entity& parent);
        void UnparentEntity(Entity& entity);
        void DestroyEntity(Entity entity);

        void OnResize(float width, float height);

        entt::registry& GetRegistry() { return mRegistry; }
        const entt::registry& GetRegistry() const { return mRegistry; }

        Pair<RuntimeCamera*, glm::mat4> GetMainCameraEntity(); // Camera - CameraTransform(view = glm::inverse(CameraTransform))
        glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);

    private:
        void ConvertToLocalSpace(Entity entity);
        void ConvertToWorldSpace(Entity entity);
        void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

    private:
        Project* mParentProject;
        SceneMetadata mMetadata;
        entt::registry mRegistry;
        bool mRuntime;
    };

    //
    // Entity
    //

    class SURGE_API Entity
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
            mScene->GetRegistry().remove<T>(mEnttHandle);
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

        UUID GetUUID()
        {
            return GetComponent<IDComponent>().ID;
        }

        UUID GetParent()
        {
            return GetComponent<ParentChildComponent>().ParentID;
        }

        Scene* GetScene() const
        {
            return mScene;
        }

        bool IsAncesterOf(Entity entity)
        {
            const auto& children = GetComponent<ParentChildComponent>().ChildIDs;

            if (children.empty())
                return false;

            for (UUID child : children)
            {
                if (child == entity.GetUUID())
                    return true;
            }

            for (UUID child : children)
            {
                if (mScene->FindEntityByUUID(child).IsAncesterOf(entity))
                    return true;
            }

            return false;
        }

        bool IsChildOf(Entity entity)
        {
            return entity.IsAncesterOf(*this);
        }

        Vector<UUID> GetChildren()
        {
            return GetComponent<ParentChildComponent>().ChildIDs;
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
