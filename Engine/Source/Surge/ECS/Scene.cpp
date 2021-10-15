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

    void Scene::CreateEntity(Entity& outEntity, const String& name)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
        outEntity.AddComponent<NameComponent>(name);
        outEntity.AddComponent<TransformComponent>();
    }

    void Scene::DestroyEntity(Entity& InEntity)
    {
        mRegistry.destroy(InEntity.Raw());
    }

} // namespace Surge
