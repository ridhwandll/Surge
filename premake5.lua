workspace "Surge"
    architecture "x64"
    startproject "SurgeSandbox"
    language "C++"
    cppdialect "C++17"

    flags
    {
        "MultiProcessorCompile"
    }

    configurations
    {
        "Debug",
        "Release"
    }

group "Dependencies"
    include "Engine/Vendor/fmt"
    include "Engine/Vendor/volk"
group ""

group "Core"
    include "Engine"
group ""

group "App"
    include "Sandbox"
group ""
