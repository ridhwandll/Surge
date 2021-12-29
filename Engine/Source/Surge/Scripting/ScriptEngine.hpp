// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Scripting/Compiler/ScriptCompiler.hpp"

namespace Surge
{
    class ScriptEngine
    {
    public:
        ScriptEngine() = default;
        ~ScriptEngine() = default;

        void Initialize();
        void Destroy();

        ScriptCompiler* GetCompiler() { return mCompiler; };

    private:
        ScriptCompiler* mCompiler;
    };

} // namespace Surge
