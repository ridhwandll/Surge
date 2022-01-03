// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Class.hpp"

namespace SurgeReflect
{
    class Registry
    {
    public:
        SURGE_API static void Initialize();
        SURGE_API static Registry* Get();
        SURGE_API static void Shutdown();

        SURGE_API ~Registry();
        SURGE_API Class* GetClass(const std::string& name);
        SURGE_API void RegisterReflectionClass(Class&& clazz);
        SURGE_API void RemoveClass(std::string name);
        SURGE_API Class* GetIfExists(const std::string& name);
        SURGE_API const auto& GetAllClasses() const { return mClasses; }

    private:
        std::unordered_map<std::string, Class*> mClasses;
    };

} // namespace SurgeReflect
