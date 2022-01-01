// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include <future>

namespace Surge
{
    struct CompileInfo
    {
        Path InputFile;
    };

    class ScriptCompiler
    {
    public:
        ScriptCompiler() = default;
        virtual ~ScriptCompiler() = default;
        SURGE_DISABLE_COPY(ScriptCompiler);

        virtual void Initialize() = 0;
        virtual void CompileAndLink(const Path& binaryDirectory, const CompileInfo& options);
        virtual void CompileAndLinkAsync(const Path& binaryDirectory, const CompileInfo& options);
        virtual const String& GetName() const = 0;
        virtual void Shutdown() = 0;

    protected:
        virtual std::wstring BuildCMDLineString(const Path& binaryDirectory, const CompileInfo& options) const = 0;
        Vector<std::future<void>> mFutures;
    };

} // namespace Surge
