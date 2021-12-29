// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Scripting/ScriptEngine.hpp"
#include "Surge/Scripting/Compiler/CompilerMSVC.hpp"
#include <future>

namespace Surge
{
    void ScriptEngine::Initialize()
    {
        mCompiler = new CompilerMSVC();
        mCompiler->Initialize("C:/Users/fahim/Desktop");

        Log<Severity::Debug>("ScriptEngine initialized with compiler: {0}", mCompiler->GetName());
        //CompileInfo opt;
        //opt.InputFile = "C:/Users/fahim/Desktop/Main.cpp";
        //mCompiler->CompileAndLink(opt);
    }

    void ScriptEngine::Destroy()
    {
        mCompiler->Shutdown();
    }

} // namespace Surge
