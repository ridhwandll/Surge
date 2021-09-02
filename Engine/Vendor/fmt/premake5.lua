project "fmt"
    location "%{wks.location}/build/Vendor/%{prj.name}"
    kind "StaticLib"
    language "C++"
    staticruntime "off"

    files
    {
        "Source/*.h",
        "Source/*.cpp",
        "Include/*.h"
    }

    includedirs
    {
        "Include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "full"
