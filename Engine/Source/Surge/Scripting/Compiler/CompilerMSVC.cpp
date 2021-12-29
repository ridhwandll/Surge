// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Scripting/Compiler/CompilerMSVC.hpp"
#include "Surge/Core/Process.hpp"
#include <filesystem>
#include "Surge/Utility/Filesystem.hpp"

#if defined(_WIN64)
#define HOST "Hostx64"
#else // _WIN64
#define HOST "Hostx86"
#endif // _WIN64

#define INT_DIRECTORY "Intermediate"
namespace Surge
{
    static Path FindProgramFilesX86Dir()
    {
        if (DWORD programFilesLength = GetEnvironmentVariableW(L"ProgramFiles(x86)", nullptr, 0))
        {
            std::wstring programFilesBuffer;
            programFilesBuffer.resize(programFilesLength);

            GetEnvironmentVariableW(L"ProgramFiles(x86)", programFilesBuffer.data(), programFilesLength);
            programFilesBuffer.pop_back(); // Remove extra null-terminator

            return std::filesystem::path(programFilesBuffer).string();
        }

        return Path();
    }

    static Path FindMSVCDir(const Path& programFilesX86)
    {
        const std::filesystem::path vsWhereLocation = std::filesystem::path((programFilesX86 / "Microsoft Visual Studio" / "Installer" / "vswhere.exe").Str());
        if (Filesystem::Exists(vsWhereLocation.string()))
        {
            // Run vswhere.exe to get the installation path of Visual Studio
            int res;
            const std::wstring vsWhereOutput = Process::OutputOf(vsWhereLocation.wstring() + L" -latest -property installationPath", res);
            if (res == 0)
            {
                // Trim trailing newlines
                const std::filesystem::path VisualStudioLocation(vsWhereOutput.begin(), vsWhereOutput.end() - 2);
                if (Filesystem::Exists(VisualStudioLocation))
                {
                    for (const std::filesystem::directory_entry& msvcDir : std::filesystem::directory_iterator(VisualStudioLocation / "VC" / "Tools" / "MSVC"))
                    {
                        return msvcDir.path().string();
                    }
                }
            }
        }

        return Path();
    }

    static Path FindWindowsSDKVersion(const Path& rProgramFilesX86)
    {
        for (const std::filesystem::directory_entry& rDirectory : std::filesystem::directory_iterator((rProgramFilesX86 / "Windows Kits" / "10" / "Lib").Str()))
        {
            const std::filesystem::path directoryPath = rDirectory.path();

            if (Filesystem::Exists(directoryPath / "um" / "x64" / "kernel32.lib"))
                return directoryPath.filename();
        }

        return Path();
    }

    void CompilerMSVC::Initialize(const Path& binaryDirectory)
    {
        mName = "MSVC";
        mBinaryDirectory = binaryDirectory;
        mX86dir = FindProgramFilesX86Dir();
        mMSVCDir = FindMSVCDir(mX86dir);
        mWinSDKVersion = FindWindowsSDKVersion(mX86dir);

        Filesystem::CreateOrEnsureDirectory(mBinaryDirectory / INT_DIRECTORY);
    }

    std::wstring CompilerMSVC::BuildCMDLineString(const CompileInfo& options) const
    {
        String fileName = Filesystem::GetNameWithoutExtension(options.InputFile);
        std::wstring inputFileName = std::wstring(fileName.begin(), fileName.end());

        std::wstring compileCmd;

        //
        // Compile
        //

        compileCmd += L"\"" + (mMSVCDir / "bin" / HOST / "x64" / "cl.exe").WStr() + L"\"";
        compileCmd += L" /nologo";
        compileCmd += L" /EHsc /std:c++17";

        // Set standard include directories
        {
            const std::filesystem::path WindowsSDKIncludeDir = std::filesystem::path((mX86dir / "Windows Kits" / "10" / "Include" / mWinSDKVersion).Str());
            compileCmd += L" /I\"" + (mMSVCDir / "include").WStr() + L"\"";
            compileCmd += L" /I\"" + (WindowsSDKIncludeDir / "ucrt").wstring() + L"\"";
            compileCmd += L" /I\"" + (WindowsSDKIncludeDir / "um").wstring() + L"\"";
            compileCmd += L" /I\"" + (WindowsSDKIncludeDir / "shared").wstring() + L"\"";
        }

        // Input File
        compileCmd += L" /Tp \"" + options.InputFile.WStr() + L"\"";
        compileCmd += L" /Fo\"" + (mBinaryDirectory / INT_DIRECTORY).WStr() + L"/" + inputFileName + L"\"";

        //
        // Link
        //

        // TODO: Make it always a produce dll
        compileCmd += L" /link";
        compileCmd += L" /SUBSYSTEM:CONSOLE";
        compileCmd += L" /OUT:\"" + mBinaryDirectory.WStr() + L"/" + inputFileName + L".exe\"";

        // Add standard library paths
        {
            const std::filesystem::path WindowsSDKLibraryDir = std::filesystem::path((mX86dir / "Windows Kits" / "10" / "Lib" / mWinSDKVersion).Str());

            compileCmd += L" /LIBPATH:\"" + (mMSVCDir / "lib" / "x64").WStr() + L"\"";
            compileCmd += L" /LIBPATH:\"" + (WindowsSDKLibraryDir / "um" / "x64").wstring() + L"\"";
            compileCmd += L" /LIBPATH:\"" + (WindowsSDKLibraryDir / "ucrt" / "x64").wstring() + L"\"";
        }

        // Miscellaneous options
        compileCmd += L" /NOLOGO /MACHINE:x64";

        return compileCmd;
    }

    void CompilerMSVC::Shutdown()
    {
    }

} // namespace Surge
