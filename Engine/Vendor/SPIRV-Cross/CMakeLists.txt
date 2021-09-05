include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES "Source/*.hpp" "Source/*.cpp" "Include/*.hpp")
set(INCLUDE_DIRS "Include/SPIRV-Cross")

add_library(SPIRV-Cross ${SOURCE_FILES})
include_directories(${INCLUDE_DIRS})
GroupSourcesByFolder(SPIRV-Cross)

set_target_properties (SPIRV-Cross PROPERTIES FOLDER Dependencies)