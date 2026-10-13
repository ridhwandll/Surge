// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Asset/Serializer/IAssetSerializer.hpp"

namespace Surge
{
    class Scene;
    class Entity;

    class SceneSerializer : public AssetSerializer
    {
    public:
        virtual ~SceneSerializer() = default;

        virtual void Initialize() override;
        virtual bool Serialize(Ref<Asset> asset) const override;
        virtual Ref<Asset> Deserialize(const AssetMetadata& metadata) const override;
        virtual void Shutdown() override;

        // JSON scene source (.srg), the Editor's authoring format. Every build loads the cooked SceneBinary instead
        static String ToJson(Scene& scene);
        static bool FromJson(const String& json, Scene& outScene);

        // In-memory single entity snapshot (JSON) of every serializable component except RelationshipComponent
        static String EntityToJson(Entity entity);
        // Restores an EntityToJson() snapshot onto 'entity': serializable components missing from the snapshot are removed.
        // IDComponent and RelationshipComponent are left untouched (hierarchy links are the caller's job)
        static bool EntityFromJson(const String& json, Entity entity);
    };
}
