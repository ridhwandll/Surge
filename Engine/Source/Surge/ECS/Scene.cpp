// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/ECS/Scene.hpp"
#include "Surge/ECS/Components.hpp"

namespace Surge
{
    Scene::Scene(bool runtime)
    {
    }

    Scene::~Scene()
    {
        mRegistry.clear();
    }

    void Scene::OnRuntimeStart()
    {
    }

    void Scene::OnRuntimeEnd()
    {
    }

    void Scene::Update(const EditorCamera& camera)
    {
        Renderer* renderer = SurgeCore::GetRenderer();
        renderer->BeginFrame(camera);
        {
            auto group = mRegistry.group<TransformComponent>(entt::get<MeshComponent>);
            for (auto& entity : group)
            {
                auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
                if (mesh.Mesh)
                    renderer->SubmitMesh(mesh, transform.GetTransform());
            }
        }
        {
            auto group = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
            for (auto& entity : group)
            {
                auto [light, transform] = group.get<PointLightComponent, TransformComponent>(entity);
                renderer->SubmitPointLight(light, transform.Position);
            }
        }
        renderer->EndFrame();
    }

    void Scene::Update()
    {
        Pair<RuntimeCamera*, glm::mat4> camera = GetMainCameraEntity();

        if (camera.Data1)
        {
            Renderer* renderer = SurgeCore::GetRenderer();
            renderer->BeginFrame(*camera.Data1, camera.Data2);
            auto group = mRegistry.group<TransformComponent>(entt::get<MeshComponent>);
            for (auto& entity : group)
            {
                auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
                if (mesh.Mesh)
                    renderer->SubmitMesh(mesh, transform.GetTransform());
            }
            {
                auto group = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
                for (auto& entity : group)
                {
                    auto [light, transform] = group.get<PointLightComponent, TransformComponent>(entity);
                    renderer->SubmitPointLight(light, transform.Position);
                }
            }
            renderer->EndFrame();
        }
    }

    template <typename T>
    static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const HashMap<UUID, entt::entity>& enttMap)
    {
        auto components = srcRegistry.view<T>();
        for (entt::entity srcEntity : components)
        {
            entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

            auto& srcComponent = srcRegistry.get<T>(srcEntity);
            auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
        }
    }

    void Scene::CopyTo(Scene* other)
    {
        HashMap<UUID, entt::entity> enttMap;
        auto idComponents = mRegistry.view<IDComponent>();
        for (entt::entity entity : idComponents)
        {
            UUID uuid = mRegistry.get<IDComponent>(entity).ID;
            Entity e;
            other->CreateEntityWithID(e, uuid, "");
            enttMap[uuid] = e.Raw();
        }

        CopyComponent<NameComponent>(other->mRegistry, mRegistry, enttMap);
        CopyComponent<TransformComponent>(other->mRegistry, mRegistry, enttMap);
        CopyComponent<MeshComponent>(other->mRegistry, mRegistry, enttMap);
        CopyComponent<CameraComponent>(other->mRegistry, mRegistry, enttMap);
        CopyComponent<PointLightComponent>(other->mRegistry, mRegistry, enttMap);
    }

    void Scene::CreateEntity(Entity& outEntity, const String& name)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
        outEntity.AddComponent<IDComponent>();
        outEntity.AddComponent<NameComponent>(name);
        outEntity.AddComponent<TransformComponent>();
    }

    void Scene::CreateEntityWithID(Entity& outEntity, const UUID& id, const String& name)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
        outEntity.AddComponent<IDComponent>(id);
        outEntity.AddComponent<NameComponent>(name);
        outEntity.AddComponent<TransformComponent>();
    }

    void Scene::DestroyEntity(Entity& entity)
    {
        mRegistry.destroy(entity.Raw());
    }

    void Scene::OnResize(float width, float height)
    {
        Pair<RuntimeCamera*, glm::mat4> camera = GetMainCameraEntity();
        if (camera.Data1)
            camera.Data1->SetViewportSize(width, height);
    }

    Pair<RuntimeCamera*, glm::mat4> Scene::GetMainCameraEntity()
    {
        Pair<RuntimeCamera*, glm::mat4> result = {nullptr, glm::mat4(1.0f)};
        auto view = mRegistry.view<TransformComponent, CameraComponent>();
        for (auto& entity : view)
        {
            const auto& [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);
            if (camera.Primary)
            {
                result = {&camera.Camera, transform.GetTransform()};
                break;
            }
        }
        return result;
    }
} // namespace Surge