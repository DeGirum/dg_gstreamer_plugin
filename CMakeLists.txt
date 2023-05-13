cmake_minimum_required( VERSION 3.16 )
project(dg_gstreamer_plugin)

add_compile_options(-Wno-class-memaccess -fPIC)
find_package(Doxygen)
if (NOT DOXYGEN_FOUND)
    message("Doxygen is not found on the system, documentation will not be generated.")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
add_subdirectory(CppSDK)
add_subdirectory(dgaccelerator)
