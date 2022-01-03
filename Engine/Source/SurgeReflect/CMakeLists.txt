include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES Source/*.hpp Source/*.cpp)
set(INCLUDE_DIRS
 	Include
 	${CMAKE_SOURCE_DIR}/Engine/Source
 	${CMAKE_SOURCE_DIR}/Engine/Vendor
    ${CMAKE_SOURCE_DIR}/Engine/Vendor/fmt/Include
    ${CMAKE_SOURCE_DIR}/Engine/Vendor/glm
    ${CMAKE_SOURCE_DIR}/Engine/Vendor/Optick/Include
    ${CMAKE_SOURCE_DIR}/Engine/Vendor/json/Include
 )

add_library(Surge.Reflection SHARED ${SOURCE_FILES})
target_include_directories(Surge.Reflection PUBLIC ${INCLUDE_DIRS})
set_target_properties(Surge.Reflection PROPERTIES FOLDER Engine)
GroupSourcesByFolder(Surge.Reflection)