#include "Surge/Scripting/Compiler/ScriptCompiler.hpp"
#include "Surge/Core/Process.hpp"

namespace Surge
{
    void ScriptCompiler::CompileAndLink(const CompileInfo& options)
    {
        Surge::Core::AddFrameEndCallback([&, options]() {
            std::future f = std::async(std::launch::async, [&, options]() {
                std::wstring compileCmd = BuildCMDLineString(options);
                Log<Severity::Info>("[{0}: Compilation started]", GetName());
                int exitCode = Process::ResultOf(compileCmd);
                Log<Severity::Info>("[{0}: Compilation ended with exit code: {1}]", GetName(), exitCode);
            });
            mFutures.emplace_back(std::move(f));
        });
    }

} // namespace Surge
