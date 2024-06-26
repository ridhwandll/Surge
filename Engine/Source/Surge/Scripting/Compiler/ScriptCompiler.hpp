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

    class SURGE_API ScriptCompiler
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
        const std::atomic_bool& IsCompiling() const { return mIsCompiling; }
        const std::atomic_bool& GetCompileStatus() const { return mCompileStatus; }
    protected:
        virtual std::wstring BuildCMDLineString(const Path& binaryDirectory, const CompileInfo& options) const = 0;
        std::atomic_bool mIsCompiling = false;
        std::atomic_bool mCompileStatus = false;
        Vector<std::future<void>> mFutures;
    };

} // namespace Surge
