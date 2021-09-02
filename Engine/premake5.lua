project "Surge"
    location "%{wks.location}/build/%{prj.name}"
    kind "StaticLib"
    staticruntime "off"

    pchsource "%{wks.location}/Engine/Source/Pch.cpp"
    pchheader "Pch.hpp"

    files
    {
        "Source/**.hpp",
        "Source/**.cpp",
    }

    includedirs
    {
        "Source",
        "Vendor/fmt/Include",
        "Vendor/glm",
        "Vendor/Vulkan-Headers/Include",
        "Vendor/VulkanMemoryAllocator/Include",
        "Vendor/volk"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "VK_USE_PLATFORM_WIN32_KHR",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_RADIANS",
        "NOMINMAX"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    links
    {
        "fmt",
        "volk"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "SURGE_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SURGE_RELEASE"
        runtime "Release"
        optimize "full"
