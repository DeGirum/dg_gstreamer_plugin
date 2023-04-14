cmake_minimum_required( VERSION 3.16 )
project(dg_gstreamer_plugin)

add_compile_options(-Wno-class-memaccess -fPIC)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
add_subdirectory(CppSDK)
add_subdirectory(dgaccelerator)
