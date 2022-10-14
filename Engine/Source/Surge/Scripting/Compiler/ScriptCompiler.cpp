#include "Surge/Scripting/Compiler/ScriptCompiler.hpp"
#include "Surge/Core/Process.hpp"

namespace Surge
{
    void ScriptCompiler::CompileAndLink(const Path& binaryDirectory, const CompileInfo& options)
    {
        std::wstring compileCmd = BuildCMDLineString(binaryDirectory, options);
        Log<Severity::Info>("[{0}: Compilation started]", GetName());
        mIsCompiling = true;
        int exitCode = Process::ResultOf(compileCmd);
        mIsCompiling = false;
        Log<Severity::Info>("[{0}: Compilation ended with exit code: {1}]", GetName(), exitCode);
        mCompileStatus = exitCode == 0 ? true : false;
    }

    void ScriptCompiler::CompileAndLinkAsync(const Path& binaryDirectory, const CompileInfo& options)
    {
        Surge::Core::AddFrameEndCallback([&, binaryDirectory, options]() {
            std::future f = std::async(std::launch::async, &ScriptCompiler::CompileAndLink, this, binaryDirectory, options);
            mFutures.emplace_back(std::move(f));
        });
    }

} // namespace Surge
