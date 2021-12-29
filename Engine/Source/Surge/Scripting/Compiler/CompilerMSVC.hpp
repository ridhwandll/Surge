// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Scripting/Compiler/ScriptCompiler.hpp"

namespace Surge
{
    class CompilerMSVC : public ScriptCompiler
    {
    public:
        CompilerMSVC() = default;
        virtual ~CompilerMSVC() override = default;

        virtual void Initialize(const Path& binaryDirectory) override;
        virtual const String& GetName() const override { return mName; };
        virtual void Shutdown() override;

    protected:
        virtual std::wstring BuildCMDLineString(const CompileInfo& options) const override;

    private:
        String mName;
        Path mBinaryDirectory;

        Path mX86dir;
        Path mMSVCDir;
        Path mWinSDKVersion;
    };

} // namespace Surge
