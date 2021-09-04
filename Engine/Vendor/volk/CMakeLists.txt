add_library(volk STATIC volk.c volk.h)
target_include_directories(volk PRIVATE ${CMAKE_SOURCE_DIR}/Engine/Vendor/Vulkan-Headers/Include)
set_target_properties(volk PROPERTIES FOLDER Dependencies)
