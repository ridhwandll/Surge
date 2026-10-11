// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ScriptAsset.hpp"

namespace Surge
{
    Script::Script(Vector<Byte>&& bytecode)
        : mBytecode(std::move(bytecode))
    {}

    Ref<Script> Script::Create(Vector<Byte>&& bytecode)
    {
        return Ref<Script>::Create(std::move(bytecode));
    }
}
