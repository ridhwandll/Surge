// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Core/Plugin.hpp"

namespace Surge
{
    class Win32Plugin : public Plugin
    {
    public:
        Win32Plugin(const String& path);
        virtual ~Win32Plugin();

        virtual void* LoadFunction(const String& name);
    private:
        HINSTANCE mModule;
    };
}