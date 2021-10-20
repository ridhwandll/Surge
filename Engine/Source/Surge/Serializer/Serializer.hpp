// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/ECS/Scene.hpp"

namespace Surge::Serializer
{
    template <typename T>
    void Serialize(const Path& path, Ref<T>& in)
    {
        static_assert(false);
    }

    template <typename T>
    void Deserialize(const Path& path, Ref<T>& out)
    {
        static_assert(false);
    }

    template <>
    void Serialize(const Path& path, Ref<Scene>& in);

    template <>
    void Deserialize(const Path& path, Ref<Scene>& out);

} // namespace Surge::Serializer