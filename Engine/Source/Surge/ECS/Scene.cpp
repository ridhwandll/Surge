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

    void Scene::CreateEntity(Entity& outEntity, const String& name)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
        outEntity.AddComponent<NameComponent>(name);
    }

    void Scene::DestroyEntity(Entity& InEntity)
    {
        mRegistry.destroy(InEntity.Raw());
    }

} // namespace Surge
