@echo off
:: Copyright (c) - SurgeTechnologies - All rights reserved
:: Generates the Visual Studio solution (build\Surge.slnx, ClangCL toolset) with CMake, after checking CMake and Visual Studio
:: (C++ workload + Clang tools). Re-run it after adding or removing source files: sources are collected with GLOB_RECURSE.
:: Used by CI (.github/workflows/WindowsCI.yml): no questions there, a failure exits with 1.
::
:: Usage: WindowsGenProjects.bat [/y]
::   /y           No questions, no pause at the end (automatic when the CI variable is set)
::   NO_COLOR=1   Plain output
setlocal EnableExtensions

call :InitColors

:: Before parsing the arguments: shift moves %%0 too
for %%i in ("%~dp0..") do set "REPO_DIR=%%~fi"
set "BUILD_DIR=%REPO_DIR%\build"
set "MIN_CMAKE_MAJOR=3"
set "MIN_CMAKE_MINOR=20"
set "STEP_COUNT=3"

set "ASSUME_YES="
if defined CI set "ASSUME_YES=1"
:ParseArgs
if "%~1"=="" goto :ArgsDone
if "%~1"=="/?" goto :Usage
if /i "%~1"=="/y" (set "ASSUME_YES=1") else if /i "%~1"=="-y" (set "ASSUME_YES=1") else (call :Warn "Unknown argument %~1, ignored.")
shift
goto :ParseArgs
:ArgsDone

echo.
echo %C_TITLE%============================================================%C_RESET%
echo %C_TITLE% Surge Windows project generation%C_RESET%
echo %C_TITLE%============================================================%C_RESET%
echo  %C_DIM%Repository :%C_RESET% %REPO_DIR%
echo  %C_DIM%Solution   :%C_RESET% %BUILD_DIR%\Surge.slnx
echo  %C_DIM%Toolset    :%C_RESET% ClangCL, C++20
echo %C_TITLE%============================================================%C_RESET%

:: ---------------------------------------------------------------------------------------------------------------------
call :Step 1 "CMake"
where cmake.exe >nul 2>&1 || (call :Error "CMake not found on PATH. Install 3.20 or newer from https://cmake.org/download and let it add itself to the PATH." & goto :Fail)
set "CMAKE_VERSION="
for /f "tokens=3" %%v in ('cmake --version 2^>nul ^| findstr /b /c:"cmake version"') do set "CMAKE_VERSION=%%v"
for /f "tokens=1,2 delims=.-" %%a in ("%CMAKE_VERSION%") do (set "CMAKE_MAJOR=%%a" & set "CMAKE_MINOR=%%b")
if not defined CMAKE_VERSION (call :Error "Could not read the CMake version." & goto :Fail)
if %CMAKE_MAJOR% LSS %MIN_CMAKE_MAJOR% goto :CMakeTooOld
if %CMAKE_MAJOR% EQU %MIN_CMAKE_MAJOR% if %CMAKE_MINOR% LSS %MIN_CMAKE_MINOR% goto :CMakeTooOld
call :Ok "CMake %CMAKE_VERSION%"
goto :CMakeDone

:CMakeTooOld
call :Error "CMake %CMAKE_VERSION% is too old, %MIN_CMAKE_MAJOR%.%MIN_CMAKE_MINOR% or newer is required."
goto :Fail

:CMakeDone

:: ---------------------------------------------------------------------------------------------------------------------
call :Step 2 "Visual Studio"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (call :Error "Visual Studio not found. Install Visual Studio 2022 or newer with the 'Desktop development with C++' workload." & goto :Fail)

:: The newest instance with the C++ tools, the one CMake's default Visual Studio generator picks
set "VS_PATH="
set "VS_NAME="
for /f "usebackq delims=" %%p in (`call "%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%p"
if not defined VS_PATH (call :Error "No Visual Studio with the 'Desktop development with C++' workload. Add it in the Visual Studio Installer." & goto :Fail)
for /f "usebackq delims=" %%n in (`call "%VSWHERE%" -path "%VS_PATH%" -property displayName`) do set "VS_NAME=%%n"
call :Ok "%VS_NAME%"
call :Info "%VS_PATH%"

:: CMakeLists.txt forces the ClangCL toolset
set "VS_CLANG="
for /f "usebackq delims=" %%p in (`call "%VSWHERE%" -path "%VS_PATH%" -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset -property installationPath`) do set "VS_CLANG=%%p"
if defined VS_CLANG (call :Ok "Clang tools for Windows (ClangCL toolset)" & goto :VisualStudioDone)
call :Error "The ClangCL toolset is missing. Visual Studio Installer, Modify, Individual components: add 'C++ Clang Compiler for Windows' and 'MSBuild support for LLVM (clang-cl) toolset'."
goto :Fail

:VisualStudioDone

:: ---------------------------------------------------------------------------------------------------------------------
call :Step 3 "Generate the solution"
:: A build folder configured for another Visual Studio install (an upgrade, a second edition) makes CMake stop with
:: "generator instance does not match": its cache has to go
set "CACHED_INSTANCE="
if exist "%BUILD_DIR%\CMakeCache.txt" for /f "tokens=2 delims==" %%c in ('findstr /b /c:"CMAKE_GENERATOR_INSTANCE:" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_INSTANCE=%%c"
if not defined CACHED_INSTANCE goto :Generate
set "CACHED_INSTANCE=%CACHED_INSTANCE:/=\%"
if /i "%CACHED_INSTANCE%"=="%VS_PATH%" goto :Generate

call :Warn "build\ was configured for another Visual Studio: %CACHED_INSTANCE%"
call :Confirm "Delete its CMake cache (build\CMakeCache.txt, build\CMakeFiles) and configure again?" || (call :Error "CMake can't reuse that build folder with %VS_NAME%." & goto :Fail)
del /q "%BUILD_DIR%\CMakeCache.txt" 2>nul
if exist "%BUILD_DIR%\CMakeFiles" rmdir /s /q "%BUILD_DIR%\CMakeFiles"
call :Ok "Cache deleted"

:Generate
call :Info "cmake -S %REPO_DIR% -B %BUILD_DIR%"
echo.
cmake -S "%REPO_DIR%" -B "%BUILD_DIR%"
if errorlevel 1 (echo. & call :Error "CMake failed, see the output above." & goto :Fail)
if not exist "%BUILD_DIR%\Surge.slnx" (call :Error "CMake finished but build\Surge.slnx was not created." & goto :Fail)
echo.
call :Ok "Generated build\Surge.slnx"

echo.
echo %C_OK%============================================================%C_RESET%
echo %C_OK% Windows projects generated%C_RESET%
echo %C_OK%============================================================%C_RESET%
echo  - Open build\Surge.slnx, start project Editor. The working directory is ALWAYS the repository root.
echo  - Command line: msbuild build\Surge.slnx -p:Configuration=Release
echo  - Run this again after adding or removing source files.
echo  - Android: Scripts\AndroidSetup.bat installs the Android toolchain.
echo.

if defined ASSUME_YES exit /b 0
call :Confirm "Open the solution in Visual Studio?" && start "" "%BUILD_DIR%\Surge.slnx"
exit /b 0

:Fail
echo.
echo %C_ERR%Project generation failed, see the messages above.%C_RESET%
if not defined ASSUME_YES pause
exit /b 1

:Usage
echo Usage: WindowsGenProjects.bat [/y]
echo   /y   No questions, no pause at the end (automatic when the CI variable is set)
exit /b 0

:: =====================================================================================================================
:: Helpers. Messages are passed quoted: no ^& ^< ^> or %% in them

:InitColors
:: ANSI colors through cmd's own output (echo, set /p): Windows 10+ consoles and Windows Terminal render them.
:: Everything colored is amber, errors are red
for %%c in (RESET TITLE STEP OK WARN ERR DIM ASK BOLD) do set "C_%%c="
if defined NO_COLOR exit /b 0
for /f %%a in ('echo prompt $E^| cmd') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_TITLE=%ESC%[38;5;214m"
set "C_STEP=%ESC%[38;5;214m"
set "C_OK=%ESC%[38;5;214m"
set "C_WARN=%ESC%[38;5;214m"
set "C_ERR=%ESC%[91m"
set "C_DIM=%ESC%[38;5;214m"
set "C_ASK=%ESC%[38;5;214m"
set "C_BOLD=%ESC%[1m"
exit /b 0

:Step
echo.
echo %C_STEP%[%~1/%STEP_COUNT%]%C_RESET% %C_BOLD%%~2%C_RESET%
exit /b 0

:Ok
echo   %C_OK%OK%C_RESET% %~1
exit /b 0

:Info
echo   %C_DIM%%~1%C_RESET%
exit /b 0

:Warn
echo   %C_WARN%WARNING%C_RESET% %~1
exit /b 0

:Error
echo   %C_ERR%ERROR %~1%C_RESET%
exit /b 0

:: Prompt "question" "[keys]": printed by set /p (cmd's own output keeps the colors, choice.exe's /m would not)
:Prompt
<nul set /p "=%C_RESET%  %C_ASK%?%C_RESET% %~1 %C_ASK%%~2%C_RESET% "
exit /b 0

:: Confirm "question": errorlevel 0 = yes, 1 = no. /y answers yes
:Confirm
if not defined ASSUME_YES goto :ConfirmAsk
echo   %C_ASK%?%C_RESET% %~1 %C_DIM%yes ^(/y^)%C_RESET%
exit /b 0
:ConfirmAsk
call :Prompt "%~1" "[Y/N]"
choice /c YN /n
if errorlevel 2 exit /b 1
exit /b 0
