project "volk"
    location "%{wks.location}/build/Vendor/%{prj.name}"
    kind "StaticLib"
    language "C"
    staticruntime "off"

    files
    {
        "volk.c", "volk.h"
    }

    includedirs
    {
        "../Vulkan-Headers/Include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "full"
