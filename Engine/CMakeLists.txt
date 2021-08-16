include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES
    Source/*.cpp
    Source/*.hpp
)

set(INCLUDE_DIRS
    Source
    Vendor
)

set(LIB_LINKS)

add_library(Surge SHARED ${SOURCE_FILES})
target_include_directories(Surge PUBLIC ${INCLUDE_DIRS})
target_link_libraries(Surge PUBLIC ${LIB_LINKS})
target_precompile_headers(Surge PRIVATE "Source/Pch.hpp")

GroupSourcesByFolder(Surge)

set_property(TARGET Surge PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties (Surge PROPERTIES FOLDER Engine)

target_compile_definitions(
    Surge

    PRIVATE
    "_CRT_SECURE_NO_WARNINGS"
    "SURGE_DLL_EXPORT"

    PUBLIC
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"

    $<$<CONFIG:Debug>:SURGE_DEBUG>
    $<$<CONFIG:Release>:SURGE_RELEASE>
)
