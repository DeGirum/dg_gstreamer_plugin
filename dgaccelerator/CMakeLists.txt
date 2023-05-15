cmake_minimum_required(VERSION 3.16)

project(dgaccelerator VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)
find_package(GTest CONFIG REQUIRED)
add_compile_options(-Werror) # Treat warnings as errors


# Check if LD_LIBRARY_PATH already contains the deepstream lib location
string(FIND "$ENV{LD_LIBRARY_PATH}" "/opt/nvidia/deepstream/deepstream-6.2/lib/" LD_LIBRARY_PATH_INDEX)

# If LD_LIBRARY_PATH doesn't contain the deepstream lib location, append it to the environment variable
if(${LD_LIBRARY_PATH_INDEX} EQUAL -1)
    set(ENV{LD_LIBRARY_PATH} "/opt/nvidia/deepstream/deepstream-6.2/lib/:$ENV{LD_LIBRARY_PATH}")
endif()

# Set NVDS installation directory
set(NVDS_INSTALL_DIR "/opt/nvidia/deepstream/deepstream")

# List all libraries in deepstream.
set(NVDS_LIBS
  nvdsgst_meta
  nvdsgst_helper
  nvdsgst_smartrecord
  nvds_meta
  nvds_msgbroker
  nvds_utils
  nvbufsurface 
  nvbufsurftransform
)

# Find all libraries in list.
foreach(LIB ${NVDS_LIBS})
  find_library(${LIB}_PATH NAMES ${LIB} PATHS ${NVDS_INSTALL_DIR}/lib)
  if(${LIB}_PATH)
    set(NVDS_LIBRARIES ${NVDS_LIBRARIES} ${${LIB}_PATH})
  else()
    message(FATAL_ERROR "Unable to find lib: ${LIB}")
    set(NVDS_LIBRARIES FALSE)
    break()
  endif()
endforeach()

# Find include directories.
find_path(NVDS_INCLUDE_DIRS
  NAMES
    nvds_version.h
  HINTS
    ${NVDS_INSTALL_DIR}/sources/includes
)

# Check libraries and includes.
if (NVDS_LIBRARIES AND NVDS_INCLUDE_DIRS)
  set(NVDS_FOUND TRUE)
else()
  message(FATAL_ERROR "Unable to find NVDS")
endif()

# Set sources
set(SRCS
    dgaccelerator_lib.h
    dgaccelerator_lib.cpp
    gstdgaccelerator.h
    gstdgaccelerator.cpp
    nvdefines.h
    ../CppSDK/inc/Utilities/dg_file_utilities.h
    ../CppSDK/inc/DglibInterface/dg_model_api.h
    ../CppSDK/client/dg_client.h
    ../CppSDK/inc/Utilities/json.hpp
)

# Create library target
add_library(nvdsgst_dgaccelerator SHARED ${SRCS})


# Set include directories
target_include_directories(nvdsgst_dgaccelerator PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${NVDS_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${NVDS_INSTALL_DIR}/sources/includes
)

# Set link libraries
target_link_libraries(nvdsgst_dgaccelerator
    aiclientlib
    pthread
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${NVDS_LIBRARIES}
    ${CUDA_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Set installation paths
set(GST_INSTALL_DIR "${NVDS_INSTALL_DIR}/lib/gst-plugins/")
set(LIB_INSTALL_DIR "${NVDS_INSTALL_DIR}/lib/")

# Install target
install(TARGETS nvdsgst_dgaccelerator 
LIBRARY DESTINATION ${GST_INSTALL_DIR} 
ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

if (DOXYGEN_FOUND)
  set (DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/Docs/Doxyfile)
  add_custom_command(TARGET nvdsgst_dgaccelerator POST_BUILD
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE} 1>nul
    DEPENDS ${DOXYFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM )
endif()

enable_testing()

add_executable(
  run_tests
  ../tests/dgaccelerator_test.cpp
)
target_include_directories(run_tests PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${NVDS_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${NVDS_INSTALL_DIR}/sources/includes
)
target_link_libraries(
  run_tests
  GTest::gtest_main
  aiclientlib
  pthread
  ${OpenCV_LIBS}
  ${GSTREAMER_LIBRARIES}
  ${GSTREAMER_VIDEO_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${NVDS_LIBRARIES}
  ${CUDA_LIBRARIES}
  ${CMAKE_DL_LIBS}
)

include(GoogleTest)

# generates CTest commands that will run all tests
gtest_discover_tests(run_tests)
# installs the ./run_tests executable
install(TARGETS run_tests DESTINATION ${CMAKE_BINARY_DIR})
# gtest_add_tests(TARGET run_tests)
