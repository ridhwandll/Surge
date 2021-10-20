include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES "Source/*.hpp" "Source/*.cpp")
set(INCLUDE_DIRS "Include" ${CMAKE_SOURCE_DIR}/Engine/Vendor/Vulkan-Headers/Include)

add_library(Optick ${SOURCE_FILES})
target_include_directories(Optick PRIVATE ${INCLUDE_DIRS})
GroupSourcesByFolder(Optick)

set_target_properties(Optick PROPERTIES FOLDER Dependencies)
target_compile_definitions(Optick

    PUBLIC
    "_CRT_SECURE_NO_WARNINGS"
    "_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING"
)
