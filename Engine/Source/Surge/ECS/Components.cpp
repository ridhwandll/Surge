// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/ECS/Components.hpp"
// clang-format off
// SurgeReflect - Component Register
// Must be in Gobal Namespace
SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::NameComponent)
    .AddVariable<&Surge::NameComponent::Name>("Name")
SURGE_REFLECT_CLASS_REGISTER_END(Surge::NameComponent)

SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::TransformComponent)
    .AddVariable<&Surge::TransformComponent::Position>("Position")
    .AddVariable<&Surge::TransformComponent::Rotation>("Rotation")
    .AddVariable<&Surge::TransformComponent::Scale>("Scale")
    .AddFunction<&Surge::TransformComponent::GetTransform>("GetTransform")
SURGE_REFLECT_CLASS_REGISTER_END(Surge::TransformComponent)

SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::MeshComponent)
    .AddVariable<&Surge::MeshComponent::Mesh>("Mesh")
SURGE_REFLECT_CLASS_REGISTER_END(Surge::MeshComponent)
