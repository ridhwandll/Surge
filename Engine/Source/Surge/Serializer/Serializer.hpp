// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Project/Project.hpp"
#include "Surge/ECS/Scene.hpp"

namespace Surge::Serializer
{
    template <typename T>
    void Serialize(const Path& path, T* in)
    {
        static_assert(false);
    }
    template <typename T>
    void Deserialize(const Path& path, T* out)
    {
        static_assert(false);
    }

    template <>
    SURGE_API void Serialize(const Path& path, Scene* in);
    template <>
    SURGE_API void Deserialize(const Path& path, Scene* out);

    template <>
    SURGE_API void Serialize(const Path& path, ProjectMetadata* in);
    template <>
    SURGE_API void Deserialize(const Path& path, ProjectMetadata* out);

} // namespace Surge::Serializer