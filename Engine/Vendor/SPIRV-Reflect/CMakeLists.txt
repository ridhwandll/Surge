include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES "Source/*.c" "Include/*.h")
set(INCLUDE_DIRS Include)

add_library(SPIRV-Reflect STATIC ${SOURCE_FILES})
include_directories(${INCLUDE_DIRS})

GroupSourcesByFolder(SPIRV-Reflect)
set_target_properties(SPIRV-Reflect PROPERTIES FOLDER Dependencies)
