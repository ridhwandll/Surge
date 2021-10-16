// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/ECS/Scene.hpp"
#include "Surge/ECS/Components.hpp"

namespace Surge
{
    Scene::Scene(bool runtime)
    {
        mRenderer = SurgeCore::GetRenderer();
    }

    Scene::~Scene()
    {
        mRegistry.clear();
    }

    void Scene::Update(const EditorCamera& camera)
    {
        mRenderer->BeginFrame(camera);
        auto group = mRegistry.group<TransformComponent>(entt::get<MeshComponent>);
        for (auto& entity : group)
        {
            auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
            if (mesh.Mesh)
                mRenderer->SubmitMesh(mesh.Mesh, transform.GetTransform());
        }
        mRenderer->EndFrame();
    }

    void Scene::Update()
    {
        Pair<RuntimeCamera*, glm::mat4> camera = GetMainCamera();

        if (camera.Data1)
        {
            mRenderer->BeginFrame(*camera.Data1, camera.Data2);
            auto group = mRegistry.group<TransformComponent>(entt::get<MeshComponent>);
            for (auto& entity : group)
            {
                auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
                if (mesh.Mesh)
                    mRenderer->SubmitMesh(mesh.Mesh, transform.GetTransform());
            }
            mRenderer->EndFrame();
        }
    }

    void Scene::CreateEntity(Entity& outEntity, const String& name)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
        outEntity.AddComponent<NameComponent>(name);
        outEntity.AddComponent<TransformComponent>();
    }

    void Scene::DestroyEntity(Entity& entity)
    {
        mRegistry.destroy(entity.Raw());
    }

    void Scene::OnResize(Uint width, Uint height)
    {
        Pair<RuntimeCamera*, glm::mat4> camera = GetMainCamera();
        if (camera.Data1)
            camera.Data1->SetViewportSize(width, height);
    }

    Pair<RuntimeCamera*, glm::mat4> Scene::GetMainCamera()
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
