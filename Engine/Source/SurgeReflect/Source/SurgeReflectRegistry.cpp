// Copyright (c) - SurgeTechnologies - All rights reserved
#include "SurgeReflect/SurgeReflectRegistry.hpp"
#include <assert.h>

namespace SurgeReflect
{
    static Registry* sInstance = nullptr;

    void Registry::RegisterReflectionClass(Class&& clazz)
    {
        GetClass(clazz.GetName())->SetupClass(std::move(clazz));
    }

    void Registry::Initialize()
    {
        if (!sInstance)
            sInstance = new Registry();
    }

    SurgeReflect::Registry* Registry::Get()
    {
        if (!sInstance)
            sInstance = new Registry();

        return sInstance;
    }

    Class* Registry::GetClass(const std::string& name)
    {
        auto itr = mClasses.find(name);
        if (itr != mClasses.end())
            return itr->second;

        return mClasses.insert({name, new Class(name)}).first->second;
    }

    void Registry::RemoveClass(std::string name)
    {
        auto itr = mClasses.find(name);
        if (itr != mClasses.end())
        {
            delete itr->second;
            mClasses.erase(name);
            return;
        }
        assert(false && "Trying to remove a class that is not present!");
    }

    SurgeReflect::Class* Registry::GetIfExists(const std::string& name)
    {
        auto itr = mClasses.find(name);
        if (itr != mClasses.end())
            return itr->second;

        return nullptr;
    }

    void Registry::Shutdown()
    {
        delete Get();
    }

    Registry::~Registry()
    {
        for (auto& clazz : mClasses)
            delete clazz.second;
    }
} // namespace SurgeReflect