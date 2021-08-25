include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

set(INCLUDE_DIRS Source)
file(GLOB_RECURSE SOURCE_FILES Source/*.cpp Source/*.hpp)

add_executable(SurgeSandbox ${SOURCE_FILES})
target_link_libraries(SurgeSandbox PRIVATE Surge)
target_include_directories(SurgeSandbox PRIVATE ${INCLUDE_DIRS})

#Copy the dll to the binary directory
add_custom_command(
    TARGET SurgeSandbox
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/Engine/$<CONFIGURATION>/Surge.dll
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>
    )

GroupSourcesByFolder(SurgeSandbox)
set_property(TARGET SurgeSandbox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties (SurgeSandbox PROPERTIES FOLDER App)
