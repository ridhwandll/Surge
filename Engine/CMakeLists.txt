include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES
    Source/Surge/*.cpp
    Source/Surge/*.hpp
    Vendor/stb/stb_image.cpp
)

set(INCLUDE_DIRS
    Source
    Vendor
    Vendor/fmt/Include
    Vendor/glm
    Vendor/Vulkan-Headers/Include
    Vendor/VulkanMemoryAllocator/Include
    Vendor/volk
    Vendor/shaderc/Include
    Vendor/SPIRV-Cross/Include
    Vendor/stb
    Vendor/ImGui
    Vendor/entt/Include
    Vendor/json/Include
)

set(LIB_LINKS
    fmt
    volk
    SPIRV-Cross
    ImGui
    ${CMAKE_SOURCE_DIR}/Engine/Vendor/shaderc/Lib/shaderc_shared.lib
)

add_library(Surge STATIC ${SOURCE_FILES})
target_include_directories(Surge PUBLIC ${INCLUDE_DIRS})
target_link_libraries(Surge PUBLIC ${LIB_LINKS})

# Add Precompiled Header
target_precompile_headers(Surge PRIVATE "Source/Pch.hpp")

GroupSourcesByFolder(Surge)

set_property(TARGET Surge PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(Surge PROPERTIES FOLDER Engine)

if (WIN32)
set(PLATFORM_COMPILE_DEFS VK_USE_PLATFORM_WIN32_KHR)
endif (WIN32)

target_compile_definitions(Surge

    PUBLIC
    "_CRT_SECURE_NO_WARNINGS"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"
    "GLM_FORCE_RADIANS"
    "NOMINMAX"
    ${PLATFORM_COMPILE_DEFS}

    $<$<CONFIG:Debug>:SURGE_DEBUG>
    $<$<CONFIG:Release>:SURGE_RELEASE>
)
