project "SurgeSandbox"
    location "%{wks.location}/build/%{prj.name}"
    kind "ConsoleApp"
    language "C++"
    staticruntime "off"

    files
    {
        "Source/*.cpp", "Source/*.hpp"
    }

    includedirs
    {
        "%{wks.location}/Engine/Source",
        "%{wks.location}/Engine/Vendor/fmt/Include",
        "%{wks.location}/Engine/Vendor/glm",
        "%{wks.location}/Engine/Vendor/Vulkan-Headers/Include",
        "%{wks.location}/Engine/Vendor/VulkanMemoryAllocator/Include",
        "%{wks.location}/Engine/Vendor/volk",
        "Source"
    }

    links
    {
        "Surge"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "full"
