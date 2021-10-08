// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Scene.hpp"

namespace Surge
{
    Scene::Scene(bool runtime)
    {
    }

    Scene::~Scene()
    {
    }

    void Scene::CreateEntity(Entity& outEntity)
    {
        entt::entity e = mRegistry.create();
        outEntity = Entity(e, this);
    }

    void Scene::DestroyEntity(Entity& InEntity)
    {
        mRegistry.destroy(InEntity.Raw());
    }

} // namespace Surge
