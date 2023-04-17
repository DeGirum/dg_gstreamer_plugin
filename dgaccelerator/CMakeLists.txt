cmake_minimum_required(VERSION 3.16)

project(dgaccelerator VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

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
    ../CppSDK/inc/Utilities/dg_file_utilities.h
    ../CppSDK/inc/DglibInterface/dg_model_api.h
    ../CppSDK/client/dg_client.h
    ../CppSDK/inc/Utilities/json.hpp
)

# Create library target
add_library(nvdsgst_dgaccelerator SHARED ${SRCS})

# Set compile options
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(nvdsgst_dgaccelerator PUBLIC -Wclass-memaccess -fPIC)
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID}: compiler not supported")
endif()

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