// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class Plugin
    {
    public:
        Plugin() = default;
        virtual ~Plugin() = default;

        virtual void* LoadFunction(const String& name) = 0;
        static Ref<Plugin> Create(const String& path);
    };
}