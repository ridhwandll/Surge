// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Surge/Project/Project.hpp"

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
    void Serialize(const Path& path, Scene* in);
    template <>
    void Deserialize(const Path& path, Scene* out);

    template <>
    void Serialize(const Path& path, Project* in);
    template <>
    void Deserialize(const Path& path, Project* out);

} // namespace Surge::Serializer